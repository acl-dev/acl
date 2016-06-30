#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/lib_fiber.h"

static int __max = 10;
static int __nsend = 0;
static int __nread = 0;
static int __display = 10;

static void fiber_producer(FIBER *fiber, void *ctx)
{
	CHANNEL *chan = (CHANNEL *) ctx;

	while (__nsend < __max) {
		int ret = channel_sendul(chan, __nsend);
		__nsend++;

		if (ret <= 0) {
			printf("fiber-%d, channel_sendul error!\r\n",
				fiber_id(fiber));
			break;
		}

		if (__nsend < __display)
			printf(">>fiber-%d, send: %d %s\r\n",
				fiber_id(fiber), __nsend,
				ret > 0 ? "ok" : "error");
	}
}

static void fiber_consumer(FIBER *fiber, void *ctx)
{
	CHANNEL *chan = (CHANNEL *) ctx;

	while (__nread < __max) {
		unsigned long n = channel_recvul(chan);
		__nread++;

		if (__nread < __display)
			printf(">>fiber-%d, recv: %lu\r\n", fiber_id(fiber), n);
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -n count -s nsenders -r nreceivers"
		" -b buf_size -d display_count\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, i, nsenders = 1, nreceivers = 1, nbuf = 10;
	CHANNEL *chan;

	while ((ch = getopt(argc, argv, "hn:s:r:b:d:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			__max = (unsigned long) atol(optarg);
			break;
		case 's':
			nsenders = atoi(optarg);
			break;
		case 'r':
			nreceivers = atoi(optarg);
			break;
		case 'b':
			nbuf = atoi(optarg);
			break;
		case 'd':
			__display = atoi(optarg);
			break;
		default:
			break;
		}
	}

	printf("max_count: %d\r\n", __max);

	chan = channel_create(sizeof(unsigned long), nbuf);

	for (i = 0; i < nsenders; i++)
		fiber_create(fiber_producer, chan, 32000);

	for (i = 0; i < nreceivers; i++)
		fiber_create(fiber_consumer, chan, 32000);

	fiber_schedule();

	channel_free(chan);

	return 0;
}
