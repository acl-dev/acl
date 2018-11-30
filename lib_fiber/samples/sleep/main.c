#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/libfiber.h"

static int __fibers_count = 2;

static void sleep_main(ACL_FIBER *fiber, void *ctx)
{
	int *n = (int *) ctx;
	time_t last, now;

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
	printf("usage: %s -h [help] -c nfibers\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, i;

	while ((ch = getopt(argc, argv, "hc:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			__fibers_count = atoi(optarg);
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
