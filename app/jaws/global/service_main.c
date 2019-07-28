#include "lib_acl.h"
#include "service.h"
#include "service_conf.h"
#include "service_ipc.h"
#include "service_main.h"

/*----------------------------------------------------------------------------*/

static ACL_FIFO *__service_modules = NULL;
static SERVICE *__service = NULL;
static module_service_main_fn __service_callback = NULL;
static ACL_DLL_ENV  __dll_env;
ACL_MEM_SLICE *var_mem_slice = NULL;

static SERVICE *service_create(ACL_AIO *aio,
	ACL_FIFO *service_modules, ACL_ARGV *ip_argv)
{
	const char *myname = "service_create";
	ACL_FIFO *services;
	SERVICE *service, *curr_service = NULL;
	ACL_ITER iter;

	/* 关闭持续读功能 */
	acl_aio_set_keep_read(aio, 0);

	services = acl_fifo_new();

	/* 加载所有的服务模块 */
	acl_foreach(iter, service_modules) {
		MODULE_SERVICE *module = (MODULE_SERVICE*) iter.data;

		service = module->mod_create();
		if (service == NULL)
			continue;
		service->aio = aio;
		service->module = module;
		service->rw_timeout = 120;
		service->conn_timeout = 10;
		service_set_dns(service, aio,
				var_cfg_dns_list,
				var_cfg_dns_lookup_timeout,
				var_cfg_dns_cache_limit,
				var_cfg_hosts_list);
		/* 创建连接池缓存对象 */
		service->conn_cache = conn_cache_create(aio, var_cfg_server_conn_limit);

		if (ip_argv) {
			ACL_ITER iter2;
			int   i = 0;

			service->bind_ip_list = (char**)
				acl_mycalloc(ip_argv->argc + 1, sizeof(char*));
			service->bind_ip_index = 0;

			acl_foreach(iter2, ip_argv) {
				char *ip = (char*) iter2.data;
				service->bind_ip_list[i++] = acl_mystrdup(ip);
			}
			service->bind_ip_list[i] = NULL;
		} else {
			service->bind_ip_list = NULL;
			service->bind_ip_index = -1;
		}
		acl_fifo_push(services, service);
	}

	if (acl_fifo_size(service_modules) == 0)
		acl_msg_fatal("%s(%d): no service available", myname, __LINE__);

	/* 设置协议处理方式 */
	acl_foreach(iter, services) {
		service = (SERVICE*) iter.data;

		if (strcasecmp(var_cfg_proto_name, service->name) == 0) {
			curr_service = service;
			__service_callback = service->module->mod_main;
			break;
		}
	}

	acl_fifo_free(services, NULL);

	if (curr_service == NULL)
		acl_msg_fatal("%s(%d): unknown protocol name(%s)",
			myname, __LINE__, var_cfg_proto_name);
	if (__service_callback == NULL)
		acl_msg_fatal("%s(%d): __service_callback null",
			myname, __LINE__);
	return (curr_service);
}

static void gc_timer(int event_type acl_unused, void *context)
{
	ACL_AIO *aio = (ACL_AIO *) context;

	acl_mem_slice_delay_destroy();
}

void service_init(ACL_AIO *aio, ACL_FIFO *modules)
{
	const char *myname = "service_init";
	ACL_ARGV *ip_argv = NULL;
	ACL_ITER iter;

	if (var_cfg_bind_ip_list && strcmp(var_cfg_bind_ip_list, "") != 0) {
		ip_argv = acl_argv_split(var_cfg_bind_ip_list, ",\t ");
	}

	if (var_cfg_aio_buf_size > 0)
		acl_aio_set_rbuf_size(aio, var_cfg_aio_buf_size);

	memset(&__dll_env, 0, sizeof(ACL_DLL_ENV));
	/*
	__dll_env.logfp = acl_log_fp();
	*/
	__dll_env.mem_slice = var_mem_slice;
	if (var_mem_slice) {
		/* 设定定时器定时清理垃圾回收器 */
		acl_aio_request_timer(aio, gc_timer, aio, 2, 1);
	}

	if (__dll_env.mem_slice)
		acl_msg_info("%s(%d): use mem_slice", myname, __LINE__);
	else
		acl_msg_info("%s(%d): no use mem_slice", myname, __LINE__);

	if (var_cfg_debug_mem == 1) {
		acl_memory_debug_start();
		acl_memory_debug_stack(1);
	} else if (var_cfg_debug_mem == 2) {
		__dll_env.mmd = acl_debug_malloc_init(NULL, "service_log.txt");
	} else if (var_cfg_debug_mem == 3) {
		acl_memory_debug_start();
		acl_memory_debug_stack(1);
		__dll_env.mmd = acl_debug_malloc_init(NULL, "service_log.txt");
	}

#ifdef	JAWS_STATIC
	if (modules == NULL)
		acl_msg_fatal("%s(%d): modules null", myname, __LINE__);
	__service_modules = modules;
#else
	(void) modules;
	__service_modules = acl_fifo_new();
	service_load_all(__service_modules, var_cfg_service_dlnames);
#endif

	/* 初始化所有加载模块 */
	acl_foreach(iter, __service_modules) {
		MODULE_SERVICE *module = (MODULE_SERVICE*) iter.data;
		module->mod_init(&__dll_env, var_cfg_service_cfgdir);
	}

	if (var_cfg_nthreads > 1) {
		int   i, event_mode = acl_aio_event_mode(aio);
		SERVICE *service;

		/* 初始化 IPC 通道 */
		service_ipc_init(aio, var_cfg_nthreads);

		for (i = 0; i < var_cfg_nthreads; i++) {
			ACL_AIO *aio_thr;

			/* 创建单独运行的线程实例的异步句柄 */
			aio_thr = acl_aio_create(event_mode);
			if (var_cfg_aio_buf_size > 0)
				acl_aio_set_rbuf_size(aio, var_cfg_aio_buf_size);
			service = service_create(aio_thr, __service_modules, ip_argv);
			service_ipc_add_service(service, __service_callback);
		}
	} else {
		__service = service_create(aio, __service_modules, ip_argv);
	}

	if (ip_argv)
		acl_argv_free(ip_argv);

	/* 内存垃圾回收定时器 */
	service_set_gctimer(aio, 10);
}

void service_exit(void)
{
	const char *myname = "service_exit";

	/* XXX: 在使用内存切片方式时，如果卸载动态加载的库会出现 core 文件，奇怪:) */
	/*
	 * service_unload_all();
	 */
	acl_msg_info("%s(%d): jaws exit now", myname, __LINE__);
}

int service_main(ACL_SOCKET fd, ACL_AIO *aio)
{
	if (var_cfg_debug_mem)
		acl_msg_info("total alloc: %d", acl_mempool_total_allocated());

#ifdef ACL_UNIX
	acl_close_on_exec(fd, 1);
#endif
	if (var_cfg_nthreads > 1) {
		/* 多线程模式下，向各个实例线程发送新的连接 */
		service_ipc_add(fd);
	} else {
		ACL_VSTREAM *vstream;
		ACL_ASTREAM *astream;

		/* 在单线程模式下打开异步流 */
		vstream = acl_vstream_fdopen(fd, O_RDWR, var_cfg_aio_buf_size,
				0, ACL_VSTREAM_TYPE_SOCK);
		astream = acl_aio_open(aio, vstream);
		__service_callback(__service, astream);
	}
	return (0);
}
