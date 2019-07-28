#include "lib_acl.h"
#include "service.h"

void service_free(SERVICE *service)
{
	if (service->dns_server)
		dns_server_close(service->dns_server);
	if (service->dns_table)
		acl_htable_free(service->dns_table, NULL);
	acl_myfree(service);
}

SERVICE *service_alloc(const char *service_name, size_t size)
{
	const char *myname = "service_alloc";
	SERVICE *service;

	if (size < sizeof(SERVICE))
		acl_msg_fatal("%s(%d): size(%d) invalid",
			myname, __LINE__, size);
	service = (SERVICE *) acl_mycalloc(1, size);
	/* 设置服务名称 */
	ACL_SAFE_STRNCPY(service->name, service_name, sizeof(service->name));
	return (service);
}

void service_set_dns(SERVICE *service, ACL_AIO *aio,
	const char *dns_list, int dns_lookup_timeout,
	int dns_cache_limit, const char *hosts_list)
{
	const char *myname = "service_set_dns";
	ACL_ARGV *argv;
	ACL_ITER iter;

	/* 创建DNS域名查询对象：外挂式查询或非阻塞式查询 */

	if (!dns_list || !strcmp(dns_list, "")) {
		int nthreads = 100, idle = 60;

		/* 创建外挂式DNS查询对象 */
		service->dns_server = dns_server_create(aio, 300);
		service->dns_table = acl_htable_create(100, 0);
		/* 创建半驻留线程池对象 */
		service->wq = acl_thread_pool_create(nthreads, idle);
		return;
	}

	/* 采用直接发送DNS协议方式进行查询的对象 */

	argv = acl_argv_split(dns_list, ",; \t");

	service->dns_handle = acl_dns_create(aio, dns_lookup_timeout);
	if (dns_cache_limit > 0)
		acl_dns_open_cache(service->dns_handle, dns_cache_limit);

	/* 添加DNS服务器地址 */

	acl_foreach(iter, argv) {
		char *addr = (char*) iter.data;
		char *ptr1 = strchr(addr, ':'), *ptr2;
		int  port, netmask = 24;

		if (ptr1) {
			*ptr1++ = 0;
			ptr2 = strchr(ptr1, ':');
			if (ptr2) {
				*ptr2++ = 0;
				netmask = atoi(ptr2);
				if (netmask <= 0 || netmask >= 32)
					netmask = 24;
			}
			port = atoi(ptr1);
			if (port <= 0 || port >= 65535)
				port = 53;
		} else
			port = 53;

		acl_msg_info("%s(%d): add dns addr (%s:%d)",
				myname, __LINE__, addr, port);
		acl_dns_add_dns(service->dns_handle, addr, port, netmask);
	}

	acl_argv_free(argv);

	/* 添加 hosts 中的静态配置域名 */
	if (hosts_list && *hosts_list)
		dns_hosts_load(service->dns_handle, hosts_list);
}

static int __timer = 10;

static void service_gc_timer(int event_type acl_unused, void *context)
{
	int   n;
	ACL_AIO *aio = (ACL_AIO*) context;
	n = acl_mem_slice_gc();
}

void service_set_gctimer(ACL_AIO *aio, int timer)
{
	__timer = timer;
	acl_aio_request_timer(aio, service_gc_timer, aio, timer, 1);
}
