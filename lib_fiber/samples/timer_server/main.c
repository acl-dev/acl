#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/lib_fiber.h"

static int __rw_timeout = 0;

typedef struct {
	FIBER *fiber;
	FIBER *timer;
	ACL_VSTREAM *conn;
} FIBER_TIMER;

static void io_timer(FIBER *fiber, void *ctx)
{
	FIBER_TIMER *ft = (FIBER_TIMER *) ctx;

	assert(fiber == ft->timer);

	fiber_set_errno(ft->fiber, ETIMEDOUT);

	printf("timer-%d wakeup, set fiber-%d, errno: %d, %d\r\n",
		fiber_id(fiber), fiber_id(ft->fiber),
		ETIMEDOUT, fiber_errno(ft->fiber));

	fiber_ready(ft->fiber);
}

static void echo_client(FIBER *fiber, void *ctx)
{
	ACL_VSTREAM *cstream = (ACL_VSTREAM *) ctx;
	char  buf[8192];
	int   ret, count = 0;
	int   ntimeout = 0;
	FIBER_TIMER *ft = (FIBER_TIMER *) acl_mymalloc(sizeof(FIBER_TIMER));

	ft->fiber = fiber;
	ft->timer = fiber_create_timer(__rw_timeout * 1000, io_timer, ft);
	ft->conn  = cstream;

#define	SOCK ACL_VSTREAM_SOCK

	while (1) {
		ret = acl_vstream_gets(cstream, buf, sizeof(buf) - 1);

		if (ret == ACL_VSTREAM_EOF) {
			printf("fiber-%d, gets error: %s, %d, %d, fd: %d, "
				"count: %d\r\n", fiber_id(fiber),
				acl_last_serror(), errno, fiber_errno(fiber),
				SOCK(cstream), count);

			if (errno != ETIMEDOUT)
				break;

			if (++ntimeout > 2)
				break;

			printf("ntimeout: %d\r\n", ntimeout);
			ft->timer = fiber_create_timer(__rw_timeout * 1000,
					io_timer, ft);
		}

		fiber_reset_timer(ft->timer, __rw_timeout * 1000);
		buf[ret] = 0;
		//printf("gets line: %s", buf);

		if (acl_vstream_writen(cstream, buf, ret) == ACL_VSTREAM_EOF) {
			printf("write error, fd: %d\r\n", SOCK(cstream));
			break;
		}

		count++;
	}

	acl_myfree(ft);
	acl_vstream_close(cstream);
}

static void fiber_accept(FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx;

	for (;;) {
		ACL_VSTREAM *cstream = acl_vstream_accept(sstream, NULL, 0);
		if (cstream == NULL) {
			printf("acl_vstream_accept error %s\r\n",
				acl_last_serror());
			break;
		}

		printf("accept one, fd: %d\r\n", ACL_VSTREAM_SOCK(cstream));
		fiber_create(echo_client, cstream, 32768);
	}

	acl_vstream_close(sstream);
}

static void fiber_sleep_main(FIBER *fiber acl_unused, void *ctx acl_unused)
{
	time_t last, now;

	while (1) {
		time(&last);
		fiber_sleep(1);
		time(&now);
		printf("wakeup, cost %ld seconds\r\n", (long) now - last);
	}
}

static void fiber_sleep2_main(FIBER *fiber acl_unused, void *ctx acl_unused)
{
	time_t last, now;

	while (1) {
		time(&last);
		fiber_sleep(3);
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
		"  -q listen_queue\r\n", procname);
}

int main(int argc, char *argv[])
{
	char addr[64];
	ACL_VSTREAM *sstream;
	int   ch, enable_sleep = 0, qlen = 128;

	acl_msg_stdout_enable(1);

	snprintf(addr, sizeof(addr), "%s", "127.0.0.1:9002");

	while ((ch = getopt(argc, argv, "hs:r:Sq:")) > 0) {
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

	acl_non_blocking(ACL_VSTREAM_SOCK(sstream), ACL_NON_BLOCKING);

	printf("%s: call fiber_creater\r\n", __FUNCTION__);
	fiber_create(fiber_accept, sstream, 32768);

	if (enable_sleep) {
		fiber_create(fiber_sleep_main, NULL, 32768);
		fiber_create(fiber_sleep2_main, NULL, 32768);
	}

	printf("call fiber_schedule\r\n");
	fiber_schedule();

	return 0;
}
