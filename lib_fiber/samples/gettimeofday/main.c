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

	usleep(1000);

	acl_fiber_gettimeofday(&end1, NULL);
	gettimeofday(&end2, NULL);

	diff1 = stamp_sub(&end1, &begin1);
	diff2 = stamp_sub(&end2, &begin2);

	printf("gettimeofday: diff1=%.3f, diff2=%.3f, tv_sec=%ld, %ld,"
		" tv_usec=%ld, %ld, now=%ld\r\n", diff1, diff2, end1.tv_sec,
		end2.tv_sec, end1.tv_usec, end2.tv_usec, time(NULL));
}

static void test_benchmark(void)
{
	struct timeval begin, end, dummy;
	double diff;
	int i, max = 100000000;

	gettimeofday(&begin, NULL);
	for (i = 0; i < max; i++) {
		acl_fiber_gettimeofday(&dummy, NULL);
	}
	gettimeofday(&end, NULL);
	diff = stamp_sub(&end, &begin);
	printf(">> acl_fiber_gettimeofday: max=%d, diff=%.2f\r\n", max, diff);

	printf("\r\n");
	printf("------------------------------------------------------------\r\n");

	gettimeofday(&begin, NULL);
	for (i = 0; i < max; i++) {
		gettimeofday(&dummy, NULL);
	}
	gettimeofday(&end, NULL);
	diff = stamp_sub(&end, &begin);
	printf(">> gettimeofday: max=%d, diff=%.2f\r\n", max, diff);
}

int main(void)
{
	test_gettimeofday();

	printf("Enter any key to continue ...");
	fflush(stdout);
	getchar();

	test_benchmark();
	return 0;
}
