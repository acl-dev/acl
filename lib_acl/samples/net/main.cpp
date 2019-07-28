#include "lib_acl.h"
#include <assert.h>
#ifdef	ACL_UNIX
#include <signal.h>
#endif

static int __quit = 0;

static void end(void)
{
#ifdef ACL_MS_WINDOWS
	acl_vstream_fprintf(ACL_VSTREAM_OUT, "enter any key to quit\r\n");
	getchar();
#endif
}

static int on_close(ACL_ASTREAM *astream, void *context)
{
	ACL_FIFO *fifo = (ACL_FIFO*) context;
	
	acl_fifo_delete(fifo, astream);

	printf(">>>close client fd=%d\r\n", ACL_VSTREAM_SOCK(acl_aio_vstream(astream)));
	return (-1);
}

static int read_complete(ACL_ASTREAM *astream,
		void *context acl_unused,
		const char *data,
		int dlen)
{
	if (strncasecmp(data, "quit", strlen("quit")) == 0) {
		__quit = 1;
		return (-1);
	}

	acl_aio_writen(astream, data, dlen);
	return (0);
}

static int accept_complete(ACL_ASTREAM *client, void *context)
{
	ACL_FIFO *fifo = (ACL_FIFO *) context;

	acl_fifo_push(fifo, client);
	printf(">>> accept one client, max_fd=%d, count=%d\r\n",
		ACL_VSTREAM_SOCK(acl_aio_vstream(client)), acl_fifo_size(fifo));

	acl_aio_ctl(client,
		ACL_AIO_CTL_TIMEOUT, 0,
		ACL_AIO_CTL_READ_HOOK_ADD, read_complete, fifo,
		ACL_AIO_CTL_CLOSE_HOOK_ADD, on_close, fifo,
		ACL_AIO_CTL_END);

	acl_non_blocking(ACL_VSTREAM_SOCK(acl_aio_vstream(client)), ACL_NON_BLOCKING);
	acl_aio_gets(client);
	return (0);
}

static int vstream_server(const char *addr)
{
	const char *myname = "vstream_server";
	ACL_VSTREAM *sstream;
	ACL_ASTREAM *server, *client;
	char  ebuf[256];
	ACL_FIFO fifo;
	ACL_AIO *aio;

	acl_fifo_init(&fifo);

#if 0
	sstream = acl_vstream_listen(addr, 128);
#else
	sstream = acl_vstream_listen_ex(addr, 256, ACL_NON_BLOCKING, 0, 0);
#endif
	if (sstream == NULL) {
		printf("%s(%d): listen on %s error(%s)\r\n",
			myname, __LINE__, addr,
			acl_last_strerror(ebuf, sizeof(ebuf)));
		return (-1);
	}

#if 0
	acl_tcp_defer_accept(ACL_VSTREAM_SOCK(sstream), 0);
#endif
	printf("%s: listen %s ok\r\n", myname, addr);

#ifdef	ACL_UNIX
	aio = acl_aio_create(ACL_EVENT_POLL);
#else
	aio = acl_aio_create(ACL_EVENT_SELECT);
#endif

#if 0
	acl_aio_set_delay_sec(aio, 0);
	acl_aio_set_delay_usec(aio, 1000000);
#endif
	server = acl_aio_open(aio, sstream);
	acl_aio_ctl(server,
		ACL_AIO_CTL_ACCEPT_FN, accept_complete,
		ACL_AIO_CTL_CTX, &fifo,
		ACL_AIO_CTL_END);
	acl_aio_accept(server);

	while (1) {
		acl_aio_loop(aio);
		if (__quit)
			break;
	}

	printf(">>> curr client %d, enter any key to exit", acl_fifo_size(&fifo));
	fflush(stdout);
	getchar();

	while (1) {
		client = (ACL_ASTREAM*) acl_fifo_pop(&fifo);
		if (client == NULL)
			break;
		acl_aio_iocp_close(client);
	}

	return (0);
}

static int vstream_client(const char *server_addr, int max)
{
	const char *myname = "vstream_client";
	ACL_VSTREAM *client;
	char  ebuf[256];
	ACL_FIFO fifo;
	int   i;

	acl_fifo_init(&fifo);

	for (i = 0; i < max; i++) {
		client = acl_vstream_connect(server_addr, ACL_BLOCKING, 0, 0, 1024);
		if (client == NULL) {
			printf("%s(%d): connect addr %s error(%s)\n",
				myname, __LINE__, server_addr,
				acl_last_strerror(ebuf, sizeof(ebuf)));
			break;
		}
		acl_fifo_push(&fifo, client);
		printf(">>> connect ok, i=%d, fd=%d\r\n", i, ACL_VSTREAM_SOCK(client));
	}

	printf(">>> connect over, count=%d, enter any key to exit", acl_fifo_size(&fifo));
	fflush(stdout);
	getchar();

	while (1) {
		client = (ACL_VSTREAM*) acl_fifo_pop(&fifo);
		if (client == NULL)
			break;
		acl_vstream_close(client);
	}

	printf(">>> exit now\r\n");
	return (0);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -s server_addr -c conn_number -r [server|client]\r\n", procname);
}

int main(int argc acl_unused, char *argv[] acl_unused)
{
	char  addr[256];
	int   ch, n, f;

	acl_lib_init();
	acl_msg_stdout_enable(1);

	addr[0] = 0;
	n = 0;
	f = 0;

	while ((ch = getopt(argc, argv, "hs:c:r:")) > 0) {
		switch (ch) {
		case 's':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'c':
			n = atoi(optarg);
			break;
		case 'r':
			if (strcasecmp(optarg, "server") == 0)
				f = 0;
			else if (strcasecmp(optarg, "client") == 0)
				f = 1;
			break;
		case 'h':
		default:
			usage(argv[0]);
			exit (0);
		}
	}

	if (f == 0) {
		if (addr[0] == 0)
			snprintf(addr, sizeof(addr), "127.0.0.1:9876");
		vstream_server(addr);
	} else if (f == 1) {
		if (addr[0] == 0)
			snprintf(addr, sizeof(addr), "127.0.0.1:9876");
		if (n <= 0)
			n = 1024;
		vstream_client(addr, n);
	} else
		usage(argv[0]);

	end();
	return (0);
}
