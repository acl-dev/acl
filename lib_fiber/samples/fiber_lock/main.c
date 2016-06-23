#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "fiber/lib_fiber.h"

static int __fibers_count = 10;

static void fiber_main(FIBER *fiber, void *ctx)
{
	FIBER_LOCK *l = (FIBER_LOCK *) ctx;

	printf("fiber-%d begin to lock\r\n", fiber_id(fiber));
	fiber_lock(l);
	printf("fiber-%d lock ok\r\n", fiber_id(fiber));

	printf("fiber-%d begin sleep\r\n", fiber_id(fiber));
	fiber_sleep(1);
	printf("fiber-%d wakeup\r\n", fiber_id(fiber));

	fiber_unlock(l);
	printf("fiber-%d unlock ok\r\n", fiber_id(fiber));

	if (--__fibers_count == 0) {
		printf("--- All fibers Over ----\r\n");
		fiber_io_stop();
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -n fibers_count\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch, n = __fibers_count, i;
	FIBER_LOCK *l = fiber_lock_create();

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
		fiber_create(fiber_main, l, 32000);

	fiber_schedule();
	fiber_lock_free(l);

	return 0;
}
