#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include "fiber/lib_fiber.h"

static  int  __nfibers = 0;

/**
 * 协程入口
 */
static void fiber_main(ACL_FIBER *fiber, void *ctx acl_unused)
{
	int  in = 0, fd = dup(in), n;
	fd_set rset;
	struct timeval tv;

	acl_non_blocking(fd, ACL_NON_BLOCKING);

	while (1) {
		FD_ZERO(&rset);
		FD_SET(fd, &rset);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		/* 监控该描述符句柄是否可读 */
		n = select(fd + 1, &rset, NULL, NULL, &tv);
		if (n < 0) {
			printf("poll error: %s\r\n", acl_last_serror());
			break;
		}

		if (n == 0)
			printf("fiber-%d: select wakeup\r\n",
				acl_fiber_id(fiber));
		else
			printf("fiber-%d: fd = %d read ready: %d\r\n",
				acl_fiber_id(fiber), fd, n);

		if (FD_ISSET(fd, &rset)) {
			char buf[256];

			/* 当描述符可读时，从中读取数据 */
			n = read(fd, buf, sizeof(buf) - 1);
			if (n < 0) {
				if (errno != EWOULDBLOCK) {
					printf("fiber-%d: error %s\r\n",
						acl_fiber_id(fiber),
						acl_last_serror());
					break;
				}
				printf("fiber-%d: %s\r\n", acl_fiber_id(fiber),
					acl_last_serror());
				continue;
			} else if (n == 0) {
				printf("fiber-%d: read over\r\n",
					acl_fiber_id(fiber));
				break;
			}

			buf[n] = 0;
			printf("fiber-%d: %s", acl_fiber_id(fiber), buf);
			fflush(stdout);
		}
	}

	printf(">>>fiber-%d exit\r\n", acl_fiber_id(fiber));

	/* 当所有协程都执行完时停止协程调度过程 */
	if (--__nfibers == 0)
		acl_fiber_io_stop();
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -c fibers_count\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, n = 1, i;

	__nfibers = 1;

	while ((ch = getopt(argc, argv, "hc:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			__nfibers = atoi(optarg);
			break;
		default:
			break;
		}
	}

	/* 循环创建指定数量的协程 */
	for (i = 0; i < __nfibers; i++)
		acl_fiber_create(fiber_main, &n, 32768);

	/* 开始调度协程过程 */
	acl_fiber_schedule();

	return 0;
}
