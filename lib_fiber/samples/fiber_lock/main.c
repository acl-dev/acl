#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "fiber/lib_fiber.h"

static int __fibers_count = 10;

static void fiber_main(ACL_FIBER *fiber, void *ctx)
{
	ACL_FIBER_MUTEX *l = (ACL_FIBER_MUTEX *) ctx;

	printf("fiber-%d begin to lock\r\n", acl_fiber_id(fiber));
	acl_fiber_mutex_lock(l);
	printf("fiber-%d lock ok\r\n", acl_fiber_id(fiber));

	printf("fiber-%d begin sleep\r\n", acl_fiber_id(fiber));
	acl_fiber_sleep(1);
	printf("fiber-%d wakeup\r\n", acl_fiber_id(fiber));

	acl_fiber_mutex_unlock(l);
	printf("fiber-%d unlock ok\r\n", acl_fiber_id(fiber));

	if (--__fibers_count == 0) {
		printf("--- All fibers Over ----\r\n");
		acl_fiber_stop();
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -n fibers_count\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch, n = __fibers_count, i;
	ACL_FIBER_MUTEX *l = acl_fiber_mutex_create();

	while ((ch = getopt(argc, argv, "hn:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			n = atoi(optarg);
			break;
		default:
			break;
		}
	}

	__fibers_count = n;

	for (i = 0; i < n; i++)
		acl_fiber_create(fiber_main, l, 32000);

	acl_fiber_schedule();
	acl_fiber_mutex_free(l);

	return 0;
}
