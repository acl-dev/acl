#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/libfiber.h"

static int __event_type = FIBER_EVENT_KERNEL;
static int __stack_size	= 128000;
static int __shared_stack = 0;
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
		if (http_client(cstream, res, len) < 0) {
			break;
		}
	}

	acl_vstream_close(cstream);
}

static void fiber_accept(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx;
	int  fd;
	ACL_FIBER_ATTR attr;

	acl_fiber_attr_init(&attr);
	acl_fiber_attr_setstacksize(&attr, __stack_size);
	acl_fiber_attr_setsharestack(&attr, __shared_stack);

	for (;;) {
		ACL_VSTREAM *cstream = acl_vstream_accept(sstream, NULL, 0);
		if (cstream == NULL) {
			printf("acl_vstream_accept error %s\r\n",
				acl_last_serror());
			break;
		}

		fd = ACL_VSTREAM_SOCK(cstream);
		acl_fiber_create2(&attr, echo_client, cstream);
		printf("accept one over: %d\r\n", fd);
	}

	acl_vstream_close(sstream);
}

static void* thread_main(void *ctx)
{
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx;


	printf("%s: call fiber_creater\r\n", __FUNCTION__);
	acl_fiber_create(fiber_accept, sstream, 128000);

	printf("call fiber_schedule\r\n");
	acl_fiber_schedule_with(__event_type);
	return NULL;
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s listen_addr\r\n"
		" -e event_type[kernel|io_uring|select|poll]\r\n"
		" -t max_threads\r\n"
		" -r rw_timeout\r\n"
		" -z stack_size[default: 128000]\r\n"
		" -S [if use shared stack]\r\n",
		procname);
}

int main(int argc, char *argv[])
{
	char addr[64];
	ACL_VSTREAM *sstream;
	int  ch, nthreads = 1, i;
	ACL_ARRAY *a = acl_array_create(10);
	ACL_ITER iter;

	snprintf(addr, sizeof(addr), "%s", "127.0.0.1:9001");

	while ((ch = getopt(argc, argv, "hs:r:t:z:Se:")) > 0) {
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
		case 'e':
			if (strcasecmp(optarg, "poll") == 0) {
				__event_type = FIBER_EVENT_POLL;
			} else if (strcasecmp(optarg, "select") == 0) {
				__event_type = FIBER_EVENT_SELECT;
			} else if (strcasecmp(optarg, "io_uring") == 0) {
				__event_type = FIBER_EVENT_IO_URING;
			}
			break;
		case 't':
			nthreads = atoi(optarg);
			break;
		case 'z':
			__stack_size = atoi(optarg);
			break;
		case 'S':
			__shared_stack = 1;
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

	printf("listen %s ok, ACL_VSTREAM's size=%zd\r\n", addr, sizeof(ACL_VSTREAM));

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
