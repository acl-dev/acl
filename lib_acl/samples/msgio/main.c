#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct {
	ACL_SOCKET fd;
	int   i;
} IPC_CTX;

typedef struct {
	ACL_AIO *aio;
	ACL_MSGIO *mio;
	ACL_VSTREAM *stream;
	int   n;
	int   stop;
	acl_pthread_t tid;
} IPC;

typedef struct {
	IPC *ipc;

	ACL_MSGIO *local_mio;
	ACL_VSTREAM *local_stream;
} IPC_CLIENT;

#define	MSG_IPC_ACCEPT	100
#define	MSG_IPC_STOP	101

#if 0
#define	USE_IPC_SYNC
#endif

static ACL_MSGIO *__ipc_listener = NULL;
static IPC_CLIENT *__ipc_clients = NULL;
static ACL_AIO *__aio_listener = NULL;
static char __ipc_addr[256];

static int msg_ipc_stop(int msg_type acl_unused, ACL_MSGIO *mio acl_unused,
	const ACL_MSGIO_INFO *info acl_unused, void *arg)
{
	IPC *ipc = (IPC*) arg;
	ipc->stop = 1;
	printf("close now(%s)\n", acl_last_serror());
	return (0);
}

static int msg_ipc_accept(int msg_type acl_unused, ACL_MSGIO *mio acl_unused,
	const ACL_MSGIO_INFO *info, void *arg)
{
	IPC *ipc = (IPC*) arg;
	IPC_CTX ctx;

	memcpy(&ctx, acl_vstring_str(info->body.buf), ACL_VSTRING_LEN(info->body.buf));
	if (ipc->n % 10000 == 0)
		printf("tid: %ld, fd: %d\n", (long) acl_pthread_self(), ctx.fd);
	ipc->n++;
	return (0);
}

#ifdef	USE_IPC_SYNC
static int  service_sync_loop_read(ACL_MSGIO *mio)
{
	ACL_VSTREAM *vstream = acl_msgio_vstream(mio);
	int   i = 0;

	acl_non_blocking(ACL_VSTREAM_SOCK(vstream), ACL_BLOCKING);

	while (1) {
		char  buf[256];
		int ret = acl_vstream_read(vstream, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF) {
			printf("tid: %ld, read error(%s)\n",
				acl_pthread_self(), acl_last_serror());
			return (0);
		}
		if (0 && i++ % 10000 == 0)
			printf("tid: %ld, i: %d\n", acl_pthread_self(), i);
	}

	return (i);
}
#endif

static void *service_thread(void *arg)
{
	IPC *ipc = (IPC*) arg;
	ACL_MSGIO *mio = ipc->mio;
	ACL_AIO *aio = acl_msgio_aio(mio); 

	acl_tcp_set_rcvbuf(ACL_VSTREAM_SOCK(acl_msgio_vstream(mio)), 1024000);

#ifdef	USE_IPC_SYNC
	service_sync_loop_read(mio);
	return (NULL);
#endif

	/* 进入事件循环 */
	while (1) {
		acl_aio_loop(aio);
		if (ipc->stop) {
			printf("tid: %ld, stop now, n: %d\n",
				(long) acl_pthread_self(), ipc->n);
			break;
		}
	}

	return (NULL);
}

static void init(int nthread)
{
	acl_pthread_attr_t attr;
	int   i;

	acl_lib_init();

	__ipc_clients = (IPC_CLIENT*) acl_mycalloc(nthread, sizeof(IPC_CLIENT));

	acl_pthread_attr_init(&attr);
	acl_pthread_attr_setdetachstate(&attr, 0);

	__aio_listener = acl_aio_create(ACL_EVENT_SELECT);
	__ipc_listener = acl_msgio_listen(__aio_listener, NULL);
	assert(__ipc_listener);
	acl_msgio_addr(__ipc_listener, __ipc_addr, sizeof(__ipc_addr));
	printf("listening on: %s\n", __ipc_addr);

	for (i = 0; i < nthread; i++) {
		__ipc_clients[i].ipc = (IPC*) acl_mycalloc(1, sizeof(IPC));
		__ipc_clients[i].ipc->aio = acl_aio_create(ACL_EVENT_SELECT);
		__ipc_clients[i].ipc->mio = acl_msgio_connect(__ipc_clients[i].ipc->aio, __ipc_addr, 0);
		/*
		acl_msgio_set_noblock(__ipc_clients[i].ipc->aio, __ipc_clients[i].ipc->mio);
		*/
		acl_msgio_reg(__ipc_clients[i].ipc->mio, MSG_IPC_ACCEPT,
			msg_ipc_accept, __ipc_clients[i].ipc);
		acl_msgio_reg(__ipc_clients[i].ipc->mio, MSG_IPC_STOP,
			msg_ipc_stop, __ipc_clients[i].ipc);
		acl_msgio_reg(__ipc_clients[i].ipc->mio, ACL_MSGIO_EXCEPT,
			msg_ipc_stop, __ipc_clients[i].ipc);

		acl_pthread_create(&__ipc_clients[i].ipc->tid, &attr, service_thread, __ipc_clients[i].ipc);
		__ipc_clients[i].local_mio = acl_msgio_accept(__ipc_listener);
		assert(__ipc_clients[i].local_mio);
		__ipc_clients[i].local_stream = acl_msgio_vstream(__ipc_clients[i].local_mio);
		acl_non_blocking(ACL_VSTREAM_SOCK(__ipc_clients[i].local_stream), ACL_NON_BLOCKING);
		acl_tcp_set_sndbuf(ACL_VSTREAM_SOCK(__ipc_clients[i].local_stream), 1024000);
		acl_tcp_set_nodelay(ACL_VSTREAM_SOCK(__ipc_clients[i].local_stream));
	}
}

static void run(int nthread, int nloop)
{
	int   i, ret;
	IPC_CTX ctx;
	time_t begin = time(NULL);

	for (i = 0; i < nloop; i++) {
		memset(&ctx, 0, sizeof(ctx));
		ctx.fd = i;
#ifdef	USE_IPC_SYNC
		acl_vstream_write(__ipc_clients[i % nthread].local_stream, &ctx, sizeof(IPC_CTX));
#else
		ret = acl_msgio_send(__ipc_clients[i % nthread].local_mio, MSG_IPC_ACCEPT, &ctx, sizeof(IPC_CTX));
		if (ret < 0)
			break;
#endif
		if (i % 10000 == 0)
			printf("run(tid=%ld): i=%d\n", (long) acl_pthread_self(), i);
	}

	for (i = 0; i < nthread; i++) {
		memset(&ctx, 0, sizeof(ctx));
		ctx.fd = -1;
		acl_msgio_send(__ipc_clients[i].local_mio, MSG_IPC_STOP, &ctx, sizeof(IPC_CTX));
	}
	for (i = 0; i < nthread; i++)
		acl_pthread_join(__ipc_clients[i].ipc->tid, NULL);
	for (i = 0; i < nthread; i++)
		acl_msgio_close(__ipc_clients[i].local_mio);
	acl_myfree(__ipc_clients);
	printf(">>>time cost: %ld\n", time(NULL) - begin);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -c nthreads -n nloop\n", procname);
}

int main(int argc, char *argv[])
{
	int   nthread = 2, nloop = 1000000, ch;

	acl_msg_stdout_enable(1);

	while ((ch = getopt(argc, argv, "hc:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 'n':
			nloop = atoi(optarg);
			break;
		case 'c':
			nthread = atoi(optarg);
			break;
		default:
			break;
		}
	}

	if (nthread <= 0)
		nthread = 2;
	if (nloop <= 0)
		nloop = 1000;

	init(nthread);
	run(nthread, nloop);
	return (0);
}
