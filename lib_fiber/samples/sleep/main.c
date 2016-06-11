#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/lib_fiber.h"

static int __fibers_count = 2;

static void sleep_main(FIBER *fiber, void *ctx)
{
	int   n = *((int*) ctx);
	time_t last, now;

	printf("fiber-%d: begin sleep %d\r\n", fiber_id(fiber), n);
	time(&last);
	n = (int) fiber_sleep(n);
	time(&now);

	printf("fiber-%d: wakup, n: %d, sleep: %ld\r\n",
		fiber_id(fiber), n, (long) (now - last));

	if (--__fibers_count == 0)
		fiber_io_stop();
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -n seconds -c nfibers\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, n = 1, i;

	while ((ch = getopt(argc, argv, "hn:c:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			n = atoi(optarg);
			break;
		case 'c':
			__fibers_count = atoi(optarg);
			break;
		default:
			break;
		}
	}

	printf("sleep n: %d, fibers: %d\r\n", n, __fibers_count);

	for (i = 0; i < __fibers_count; i++)
		fiber_create(sleep_main, &n, 32768);

	fiber_schedule();

	return 0;
}
