#include <stdio.h>
#include <stdlib.h>
#include "lib_acl.h"
#include "fiber/libfiber.h"
#include "stamp.h"

static void test_gettimeofday(void)
{
	struct timeval begin1, end1;
	struct timeval begin2, end2;
	double diff1, diff2;

	acl_fiber_gettimeofday(&begin1, NULL);

	gettimeofday(&begin2, NULL);
	acl_fiber_gettimeofday(&begin1, NULL);

	usleep(100000);

	acl_fiber_gettimeofday(&end1, NULL);
	gettimeofday(&end2, NULL);

	diff1 = stamp_sub(&end1, &begin1);
	diff2 = stamp_sub(&end2, &begin2);

	printf("thread-%ld: gettimeofday: diff1=%.3f, diff2=%.3f, tv_sec=%ld,"
		" %ld, tv_usec=%ld, %ld, now=%ld\r\n", (long) pthread_self(),
		diff1, diff2, end1.tv_sec, end2.tv_sec, end1.tv_usec,
		end2.tv_usec, time(NULL));
}

static void test_benchmark(void)
{
	struct timeval begin, end, dummy;
	struct timespec ts;
	double diff;
	int i, max = 100000000;

	test_gettimeofday();

	gettimeofday(&begin, NULL);
	for (i = 0; i < max; i++) {
		acl_fiber_gettimeofday(&dummy, NULL);
	}
	gettimeofday(&end, NULL);
	diff = stamp_sub(&end, &begin);
	printf("thread-%ld: acl_fiber_gettimeofday: max=%d, diff=%.2f\r\n",
		(long) pthread_self(), max, diff);

	gettimeofday(&begin, NULL);
	for (i = 0; i < max; i++) {
		gettimeofday(&dummy, NULL);
	}
	gettimeofday(&end, NULL);
	diff = stamp_sub(&end, &begin);
	printf("thread-%ld: gettimeofday: max=%d, diff=%.2f\r\n",
		(long) pthread_self(), max, diff);

	gettimeofday(&begin, NULL);
	for (i = 0; i < max; i++) {
		clock_gettime(CLOCK_REALTIME, &ts);
	}
	gettimeofday(&end, NULL);
	diff = stamp_sub(&end, &begin);
	printf("thread-%ld: clock_gettime: max=%d, diff=%.2f\r\n",
		(long) pthread_self(), max, diff);
}

static void *bench_thread(void *arg)
{
	(void) arg;
	test_benchmark();
	return NULL;
}

static void usage(const char *progname)
{
	printf("usage: %s -h [help] -c nthreads\r\n", progname);
}

int main(int argc, char *argv[])
{
#define MAX	10
	int i, ch, nthreads = 1;
	pthread_t tids[MAX];

	test_gettimeofday();

	while ((ch = getopt(argc, argv, "hc:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			nthreads = atoi(optarg);
			if (nthreads <= 0 || nthreads > MAX) {
				nthreads = 1;
			}
			break;
		default:
			break;
		}
	}

	for (i = 0; i < nthreads; i++) {
		pthread_create(&tids[i], NULL, bench_thread, NULL);
	}

	for (i = 0; i < nthreads; i++) {
		pthread_join(tids[i], NULL);
	}
	return 0;
}
