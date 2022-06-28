#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "lib_protocol.h"

static int connect_callback(ACL_ASTREAM *stream, void *context);

static int close_callback(ACL_ASTREAM *stream, void *context)
{
	const char *myname = "close_callback";
	ACL_AIO *aio = (ACL_AIO*) context;
	const char *addr = ACL_VSTREAM_PATH(stream->stream);

	printf("%s: re-connect %s, aio(%s)\n", myname, addr, aio ? "not null" : "null");
	stream = acl_aio_connect(aio, addr, 0);
	if (stream == NULL) {
		printf("%s: connect addr(%s) error(%s)\n",
			myname, addr, acl_last_serror());
		return (-1);
	}
	printf("%s: re-connect %s ok\n", myname, addr);
	acl_aio_ctl(stream, ACL_AIO_CTL_CONNECT_FN, connect_callback,
		ACL_AIO_CTL_CTX, aio,
		ACL_AIO_CTL_END);
	return (-1);
}

static int read_callback(ACL_ASTREAM *stream acl_unused, void *context acl_unused)
{
	return (0);
}

static int connect_callback(ACL_ASTREAM *stream, void *context)
{
	const char *myname = "connect_callback";

	acl_aio_ctl(stream, ACL_AIO_CTL_READ_HOOK_ADD, read_callback, context,
			ACL_AIO_CTL_CLOSE_HOOK_ADD, close_callback, context,
			ACL_AIO_CTL_END);
	printf("%s: connect %s ok, fd(%d)\n", myname,
		ACL_VSTREAM_PATH(stream->stream),
		ACL_VSTREAM_SOCK(stream->stream));
	acl_aio_read(stream);
	return (0);
}

static void test_connect(const char *addr, int n)
{
	int   i;
	ACL_AIO *aio;
	ACL_ASTREAM *stream;

#ifdef	ACL_UNIX
	aio = acl_aio_create(ACL_EVENT_KERNEL);
#else
	aio = acl_aio_create(ACL_EVENT_SELECT);
#endif

	for (i = 0; i < n; i++) {
		stream = acl_aio_connect(aio, addr, 0);
		if (stream == NULL) {
			printf("connect addr(%s) error(%s), i(%d)\n",
				addr, acl_last_serror(), i);
			break;
		}

		acl_aio_ctl(stream, ACL_AIO_CTL_CONNECT_FN, connect_callback,
			ACL_AIO_CTL_CTX, aio,
			ACL_AIO_CTL_END);
	}

	while (1) {
		acl_aio_loop(aio);
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -s server_addr -n connect_cocurrent\n", procname);
}

int main(int argc, char *argv[])
{
	char  addr[128] = "127.0.0.1:8284";
	int   ch, n = 1000;

	acl_init();

	while ((ch = getopt(argc, argv, "hs:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 's':
			ACL_SAFE_STRNCPY(addr, optarg, sizeof(addr));
			break;
		case 'n':
			n = atoi(optarg);
			if (n <= 0)
				n = 100;
			break;
		default:
			break;
		}
	}

	test_connect(addr, n);
	return (0);
}
