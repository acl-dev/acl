#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include "fiber/lib_fiber.h"

static  int  __nfibers = 0;

static void select_sleep(ACL_FIBER *fiber, void *ctx acl_unused)
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
	if (--__nfibers == 0)
		acl_fiber_io_stop();
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -a cmd -c fibers_count\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, n = 1, i;
	char  cmd[128];

	snprintf(cmd, sizeof(cmd), "sleep");
	__nfibers = 1;

	while ((ch = getopt(argc, argv, "ha:c:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'a':
			snprintf(cmd, sizeof(cmd), "%s", optarg);
			break;
		case 'c':
			__nfibers = atoi(optarg);
			break;
		default:
			break;
		}
	}

	for (i = 0; i < __nfibers; i++)
		acl_fiber_create(select_sleep, &n, 32768);

	acl_fiber_schedule();

	return 0;
}
