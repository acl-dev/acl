#include "lib_acl.h"
#include "service_struct.h"
#include "service.h"
#include "service_conf.h"
#include "service_ipc.h"

typedef struct {
	acl_pthread_t tid;
	ACL_MSGIO *mio;
	ACL_AIO *aio;
	SERVICE *service;
	int   n;
} IPC_THREAD;

typedef struct {
	ACL_SOCKET fd;
	ACL_AIO *aio;
	int   i;
} IPC_CTX;

#define	MSG_LISTENER_ACCEPT	(ACL_MSGIO_USER + 100)
#define	MSG_IPC_ACCEPT		(ACL_MSGIO_USER + 101)

static IPC_THREAD *__ipc_threads = NULL;
static int   __ipc_nthread = 0;
static int   __ipc_ithread = 0;

static ACL_MSGIO *__ipc_listener = NULL;
static char __ipc_addr[256];
static module_service_main_fn __service_callback = NULL;

/* 某个线程服务实例接收到一个新的远程客户端连接请求 */

static int msg_ipc_accept(int msg_type acl_unused, ACL_MSGIO *mio acl_unused,
        const ACL_MSGIO_INFO *info, void *arg)
{
	IPC_CTX ctx;
	ACL_VSTREAM *stream;
	ACL_ASTREAM *client;
	SERVICE *service = (SERVICE*) arg;

	memcpy(&ctx, acl_vstring_str(info->body.buf), ACL_VSTRING_LEN(info->body.buf));

	/* 打开异步流 */
	stream = acl_vstream_fdopen(ctx.fd, O_RDWR, var_cfg_aio_buf_size,
			0, ACL_VSTREAM_TYPE_SOCK);
	client = acl_aio_open(ctx.aio, stream);

	/* 开始处理该客户端连接 */
	__service_callback(service, client);
	return (0);
}

/* 单线程实例非阻塞处理过程 */

static void *service_thread(void *arg)
{
	ACL_MSGIO *ipc_client = (ACL_MSGIO*) arg;
	ACL_AIO *aio = acl_msgio_aio(ipc_client);

	/* 内存垃圾回收定时器 */
	service_set_gctimer(aio, 10);

	/* 进入事件循环 */
	while (1) {
		acl_aio_loop(aio);
	}

	return (NULL);
}

/* 主线程收到新的连接到达消息 */

void service_ipc_init(ACL_AIO *aio, int nthreads)
{
	const char *myname = "service_ipc_init";

	__ipc_nthread = nthreads;
	__ipc_threads = (IPC_THREAD*) acl_mycalloc(nthreads, sizeof(IPC_THREAD));

	__ipc_listener = acl_msgio_listen(aio, NULL);
	if (__ipc_listener == NULL)
		acl_msg_fatal("%s(%d): acl_msgio_listen error(%s)",
			myname, __LINE__, acl_last_serror());
	acl_msgio_addr(__ipc_listener, __ipc_addr, sizeof(__ipc_addr));
}

void service_ipc_add_service(SERVICE *service,
	module_service_main_fn service_callback)
{
	const char *myname = "service_ipc_add_service";
	acl_pthread_attr_t attr;
	ACL_AIO *aio = service->aio;
	ACL_MSGIO *ipc_client;

	if (service_callback == NULL)
		acl_msg_fatal("%s(%d): service_callback null", myname, __LINE__);
	__service_callback = service_callback;

	if (__ipc_ithread >= __ipc_nthread)
		acl_msg_fatal("%s(%d): __ipc_ithread(%d) >= __ipc_nthread(%d)",
			myname, __LINE__, __ipc_ithread, __ipc_nthread);

	/* 创建非阻塞线程池实例，每个线程为一个单独的非阻塞实例 */

	acl_pthread_attr_init(&attr);
	acl_pthread_attr_setdetachstate(&attr, 1);

	/* 连接消息服务器 */
#if 1
	ipc_client = acl_msgio_connect(aio, __ipc_addr, 0);
	if (ipc_client == NULL)
		acl_msg_fatal("%s(%d): connect server(%s) error(%s)",
			myname, __LINE__, __ipc_addr, acl_last_serror());
#else
	ipc_client = acl_msgio_connect(NULL, __ipc_addr, 0);
	if (ipc_client == NULL)
		acl_msg_fatal("%s(%d): connect server(%s) error(%s)",
			myname, __LINE__, __ipc_addr, acl_last_serror());
	/* 设为非阻塞模式 */
	acl_msgio_set_noblock(aio, ipc_client);
#endif
	/* 注册消息事件 */
	acl_msgio_reg(ipc_client, MSG_IPC_ACCEPT, msg_ipc_accept, service);

	/* 记录服务端IPC通道 */
	__ipc_threads[__ipc_ithread].mio = acl_msgio_accept(__ipc_listener);
	__ipc_threads[__ipc_ithread].aio = aio;
	__ipc_threads[__ipc_ithread].service = service;
	acl_non_blocking(ACL_VSTREAM_SOCK(acl_msgio_vstream(__ipc_threads[__ipc_ithread].mio)), ACL_BLOCKING);
	/* 创建单线程非阻塞实例 */
	acl_pthread_create(&__ipc_threads[__ipc_ithread].tid, &attr,
		service_thread, ipc_client);
	__ipc_ithread++;
}

void service_ipc_add(ACL_SOCKET fd)
{
	IPC_CTX ctx;

	/* 轮循下一个实例线程 */
	if (__ipc_ithread >= __ipc_nthread)
		__ipc_ithread = 0;

	/* 构造消息数据 */

	ctx.fd = fd; 
	ctx.aio = __ipc_threads[__ipc_ithread].aio;

	/* 发送消息 */
	acl_msgio_send(__ipc_threads[__ipc_ithread].mio,
		MSG_IPC_ACCEPT, &ctx, sizeof(IPC_CTX));
	__ipc_threads[__ipc_ithread].n++;
	__ipc_ithread++;
}
