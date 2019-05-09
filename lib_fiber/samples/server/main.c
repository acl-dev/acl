#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/libfiber.h"

static int __stack_size = 320000;
static int __rw_timeout = 0;
static int __echo_data  = 0;

static void echo_client(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *cstream = (ACL_VSTREAM *) ctx;
	char  buf[8192];
	int   ret, count = 0;

	cstream->rw_timeout = __rw_timeout;

#define	SOCK ACL_VSTREAM_SOCK

	while (1) {
		ret = acl_vstream_gets(cstream, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF) {
			//printf("gets error: %s, fd: %d, count: %d\r\n",
			//	acl_last_serror(), SOCK(cstream), count);
			break;
		}
		buf[ret] = 0;
		//printf("gets line: %s", buf);

		if (!__echo_data) {
			count++;
			continue;
		}

		if (acl_vstream_writen(cstream, buf, ret) == ACL_VSTREAM_EOF) {
			printf("write error, fd: %d\r\n", SOCK(cstream));
			break;
		}

		count++;
	}

	acl_vstream_close(cstream);
}

static void fiber_accept(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx;

	for (;;) {
		ACL_VSTREAM *cstream = acl_vstream_accept(sstream, NULL, 0);
		if (cstream == NULL) {
			printf("acl_vstream_accept error %s\r\n",
				acl_last_serror());
			break;
		}

		//printf("accept one, fd: %d\r\n", ACL_VSTREAM_SOCK(cstream));
		acl_fiber_create(echo_client, cstream, __stack_size);
		//printf("continue to accept\r\n");
	}

	acl_vstream_close(sstream);
}

static void fiber_sleep_main(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
	time_t last, now;

	while (1) {
		time(&last);
		acl_fiber_sleep(1);
		time(&now);
		printf("wakeup, cost %ld seconds\r\n", (long) now - last);
	}
}

static void fiber_sleep2_main(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
	time_t last, now;

	while (1) {
		time(&last);
		acl_fiber_sleep(3);
		time(&now);
		printf(">>>wakeup, cost %ld seconds<<<\r\n", (long) now - last);
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		"  -s listen_addr\r\n"
		"  -r rw_timeout\r\n"
		"  -S [if sleep]\r\n"
		"  -q listen_queue\r\n"
		"  -z stack_size\r\n"
		"  -w [if echo data, default: no]\r\n", procname);
}

int main(int argc, char *argv[])
{
	char addr[64];
	ACL_VSTREAM *sstream;
	int   ch, enable_sleep = 0, qlen = 128;

	acl_msg_stdout_enable(1);

	snprintf(addr, sizeof(addr), "%s", "127.0.0.1:9002");

	while ((ch = getopt(argc, argv, "hs:r:Sq:wz:")) > 0) {
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
		case 'S':
			enable_sleep = 1;
			break;
		case 'q':
			qlen = atoi(optarg);
			break;
		case 'w':
			__echo_data = 1;
			break;
		case 'z':
			__stack_size = atoi(optarg);
			break;
		default:
			break;
		}
	}

	sstream = acl_vstream_listen(addr, qlen);
	if (sstream == NULL) {
		printf("acl_vstream_listen error %s\r\n", acl_last_serror());
		return 1;
	}

	printf("listen %s ok\r\n", addr);

	printf("%s: call fiber_creater\r\n", __FUNCTION__);
	acl_fiber_create(fiber_accept, sstream, 327680);

	if (enable_sleep) {
		acl_fiber_create(fiber_sleep_main, NULL, 327680);
		acl_fiber_create(fiber_sleep2_main, NULL, 327680);
	}

	printf("call fiber_schedule\r\n");
	acl_fiber_schedule();

	return 0;
}
