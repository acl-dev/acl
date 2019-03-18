#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/libfiber.h"

#define	STACK_SIZE	160000

static int __rw_timeout = 0;

static int http_client(ACL_VSTREAM *cstream, const char* res, size_t len)
{
	char  buf[8192];
	int   ret;

	cstream->rw_timeout = __rw_timeout;

	while (1) {
		ret = acl_vstream_gets(cstream, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF) {
			printf("gets error: %s\r\n", acl_last_serror());
			return -1;
		}

		buf[ret] = 0;

		if (strcmp(buf, "\r\n") == 0 || strcmp(buf, "\n") == 0)
			break;
	}

	if (acl_vstream_writen(cstream, res, len) == ACL_VSTREAM_EOF) {
		printf("write error\r\n");
		return -1;
	}

	return 0;
}

static void echo_client(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *cstream = (ACL_VSTREAM *) ctx;
	const char* res = "HTTP/1.1 200 OK\r\n"
		"Date: Tue, 31 May 2016 14:20:28 GMT\r\n"
		"Content-Length: 12\r\n"
		"Connection: Keep-Alive\r\n"
		"\r\n"
		"hello world!";
	size_t len = strlen(res);

	while (1) {
		if (http_client(cstream, res, len) < 0)
			break;
	}

	acl_vstream_close(cstream);
}

static void fiber_accept(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx;
	int  fd;

	for (;;) {
		ACL_VSTREAM *cstream = acl_vstream_accept(sstream, NULL, 0);
		if (cstream == NULL) {
			printf("acl_vstream_accept error %s\r\n",
				acl_last_serror());
			break;
		}

		fd = ACL_VSTREAM_SOCK(cstream);
		acl_fiber_create(echo_client, cstream, STACK_SIZE);
		printf("accept one over: %d\r\n", fd);
	}

	acl_vstream_close(sstream);
}

static void* thread_main(void *ctx)
{
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx;

	printf("%s: call fiber_creater\r\n", __FUNCTION__);
	acl_fiber_create(fiber_accept, sstream, STACK_SIZE);

	printf("call fiber_schedule\r\n");
	acl_fiber_schedule();
	return NULL;
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s listen_addr\r\n"
		" -t max_threads\r\n"
		" -r rw_timeout\r\n", procname);
}

int main(int argc, char *argv[])
{
	char addr[64];
	ACL_VSTREAM *sstream;
	int  ch, nthreads = 1, i;
	ACL_ARRAY *a = acl_array_create(10);
	ACL_ITER iter;

	snprintf(addr, sizeof(addr), "%s", "127.0.0.1:9001");

	while ((ch = getopt(argc, argv, "hs:r:t:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'r':
			__rw_timeout = atoi(optarg);
			break;
		case 't':
			nthreads = atoi(optarg);
			break;
		default:
			break;
		}
	}

	sstream = acl_vstream_listen(addr, 128);
	if (sstream == NULL) {
		printf("acl_vstream_listen error %s\r\n", acl_last_serror());
		return 1;
	}
	printf("listen %s ok\r\n", addr);

	for (i = 0; i < nthreads; i++) {
		acl_pthread_t* tid = acl_mymalloc(sizeof(acl_pthread_t));
		if (acl_pthread_create(tid, NULL, thread_main, sstream) != 0) {
			printf("create thread failed!\r\n");
			return 1;
		}

		acl_array_append(a, tid);
	}

	acl_foreach(iter, a) {
		acl_pthread_t *tid = (acl_pthread_t *) iter.data;
		if (acl_pthread_join(*tid, NULL) != 0) {
			printf("pthread join failed!\r\n");
			return 1;
		}
		acl_myfree(tid);
	}

	acl_array_free(a, NULL);
	return 0;
}
