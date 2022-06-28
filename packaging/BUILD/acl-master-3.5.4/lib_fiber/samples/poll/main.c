#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include "fiber/libfiber.h"

static  int  __nfibers = 0;

static void poll_sleep(ACL_FIBER *fiber, void *ctx acl_unused)
{
	int  in = 0, fd = dup(in), n;
	struct pollfd pfd;

	//fd = in;
	memset(&pfd, 0, sizeof(pfd));
	acl_non_blocking(fd, ACL_NON_BLOCKING);
	pfd.fd = fd;
	pfd.events = POLLIN;

	while (1) {
		n = poll(&pfd, 1, 10000);
		if (n < 0) {
			printf("poll error: %s\r\n", acl_last_serror());
			break;
		}

		if (n == 0) {
			printf("fiber-%d: fd=%d, poll wakeup\r\n",
				acl_fiber_id(fiber), fd);
		} else
			printf("fiber-%d: fd = %d read ready %s\r\n",
				acl_fiber_id(fiber), pfd.fd,
				pfd.revents & POLLIN ? "yes" : "no");

		if (pfd.revents & POLLIN) {
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
			pfd.revents = 0;
		}
	}

	close(fd);
	printf(">>>fiber-%d close %d exit\r\n", acl_fiber_id(fiber), fd);

	if (--__nfibers == 0) {
		printf("All are over!\r\n");
		//acl_fiber_schedule_stop();
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, n = 1;

	while ((ch = getopt(argc, argv, "h")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			break;
		}
	}

	acl_fiber_msg_stdout_enable(1);

	__nfibers++;
	acl_fiber_create(poll_sleep, &n, 32768);

	__nfibers++;
	acl_fiber_create(poll_sleep, &n, 32768);

	__nfibers++;
	acl_fiber_create(poll_sleep, &n, 32768);

	__nfibers++;
	acl_fiber_create(poll_sleep, &n, 32768);

	acl_fiber_schedule();

	return 0;
}
