#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include "fiber/libfiber.h"

static  int  __nfibers = 0;

/**
 * 鍗忕▼鍏ュ彛
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

		/* 鐩戞帶璇ユ弿杩扮鍙ユ焺鏄惁鍙 */
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

			/* 褰撴弿杩扮鍙鏃讹紝浠庝腑璇诲彇鏁版嵁 */
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

	/* 褰撴墍鏈夊崗绋嬮兘鎵ц瀹屾椂鍋滄鍗忕▼璋冨害杩囩▼ */
	if (--__nfibers == 0) {
		printf("All are over!\r\n");
		//acl_fiber_schedule_stop();
	}
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

	acl_fiber_msg_stdout_enable(1);

	/* 寰幆鍒涘缓鎸囧畾鏁伴噺鐨勫崗绋� */
	for (i = 0; i < __nfibers; i++)
		acl_fiber_create(fiber_main, &n, 327680);

	/* 寮€濮嬭皟搴﹀崗绋嬭繃绋� */
	acl_fiber_schedule();

	return 0;
}
