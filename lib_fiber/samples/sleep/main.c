#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include "fiber/libfiber.h"

static int __fibers_count = 2;
static int __sleep_count  = 10;

enum {
	SELECT_SLEEP = 1,
	POLL_SLEEP   = 2,
};

static int __sleep_way = 0;

static void select_sleep(int seconds)
{
	struct timeval tv;

	tv.tv_sec  = seconds;
	tv.tv_usec = 0;
	(void) select(0, NULL, NULL, NULL, &tv);
}

static void poll_sleep(int seconds)
{
	int delay = seconds * 1000;
	(void) poll(NULL, 0, delay);
}

static void sleep_main(ACL_FIBER *fiber, void *ctx)
{
	int *n = (int *) ctx, i;
	time_t last, now;

	for (i = 0; i < __sleep_count; i++) {
		switch (__sleep_way) {
		case SELECT_SLEEP:
			select_sleep(1);
			break;
		case POLL_SLEEP:
			poll_sleep(1);
			break;
		default:
			acl_fiber_sleep(1);
			break;
		}
		printf("fiber-%d wakeup %d\r\n", acl_fiber_self(), i);
	}

	printf("fiber-%d: begin sleep %d\r\n", acl_fiber_id(fiber), *n);
	time(&last);
	*n = (int) acl_fiber_sleep(*n);
	time(&now);

	printf("fiber-%d: wakup, n: %d, sleep: %ld\r\n",
		acl_fiber_id(fiber), *n, (long) (now - last));

	acl_myfree(n);

	if (--__fibers_count == 0) {
		printf("All are over!\r\n");
		//acl_fiber_schedule_stop();
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -c nfibers\r\n"
		" -n sleep_count\r\n"
		" -s sleep_way[select|poll|normal]\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, i;

	while ((ch = getopt(argc, argv, "hc:s:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			__fibers_count = atoi(optarg);
			break;
		case 's':
			if (strcasecmp(optarg, "select") == 0) {
				__sleep_way = SELECT_SLEEP;
			} else if (strcasecmp(optarg, "poll") == 0) {
				__sleep_way = POLL_SLEEP;
			}
			break;
		case 'n':
			__sleep_count = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl_fiber_msg_stdout_enable(1);

	printf("fibers: %d\r\n", __fibers_count);

	for (i = 1; i <= __fibers_count; i++) {
		int *n = (int *) acl_mymalloc(sizeof(int));
		*n = i;
		acl_fiber_create(sleep_main, n, 327680);
	}

	acl_fiber_schedule();

	return 0;
}
