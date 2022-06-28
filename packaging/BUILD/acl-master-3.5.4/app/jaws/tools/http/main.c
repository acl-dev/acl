#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "lib_protocol.h"

static void test_connect(const char *addr)
{
	ACL_VSTREAM *stream;
#if 1
	const char *request = "GET / HTTP/1.1\nHOST: store.hexun.com\nConnection: keep-alive\n\nGET / HTTP/1.1\nHOST: store.hexun.com\nConnection: close\n\n";
#else
	const char *request = "GET / HTTP/1.1\nHOST: store.hexun.com\nConnection: keep-alive\n\n";
#endif

	stream = acl_vstream_connect(addr, ACL_BLOCKING, 10, 10, 4096);
	if (stream == NULL) {
		printf("connect %s error(%s)\n", addr, acl_last_serror());
		return;
	}
	acl_vstream_fprintf(stream, "%s", request);
	printf("request:(%s)\n", request);

	while (1) {
		char  buf[4096];
		int   ret = acl_vstream_read(stream, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF)
			break;
		buf[ret] = 0;
		printf("%s", buf);
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -s server_addr\n", procname);
}

static void test(void)
{
	int   i, n = 0;

	ACL_METER_TIME("---begin---");
	for (i = 0; i < 10000; i++) {
		if (i % 100 == 0)
			n++;
	}
	ACL_METER_TIME("---end---");

	exit (0);
}

int main(int argc, char *argv[])
{
	char  addr[128] = "127.0.0.1:8283";
	int   ch;

	acl_init();

	test();

	while ((ch = getopt(argc, argv, "hs:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 's':
			ACL_SAFE_STRNCPY(addr, optarg, sizeof(addr));
			break;
		default:
			break;
		}
	}

	test_connect(addr);
	return (0);
}
