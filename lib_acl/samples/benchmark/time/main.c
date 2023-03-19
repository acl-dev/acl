#include "lib_acl.h"
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>
#include "../stamp.h"

static void usage(const char *proc) {
	printf("usage: %s -h [help] -n count\r\n", proc);
}

int main(int argc, char *argv[]) {
	int ch, i, n = 100;
	struct timeval begin, end;
	double cost, speed;

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

	gettimeofday(&begin, NULL);

	for (i = 0; i < n; i++) {
		struct timeval now;
		gettimeofday(&now, NULL);
	}

	gettimeofday(&end, NULL);

	cost = stamp_sub(&end, &begin);
	speed = (n * 1000) / (cost >= 1.0 ? cost : 1.0);
	printf("gettimeofday: total count=%d, cost=%.2f ms, speed=%.2f\r\n",
		n, cost, speed);

	gettimeofday(&begin, NULL);

	for (i = 0; i < n; i++) {
		//time_t now;
		//time(&now);
		struct timespec now;
		//clock_gettime(CLOCK_REALTIME, &now);
		clock_gettime(CLOCK_REALTIME_COARSE, &now);
		//clock_gettime(CLOCK_THREAD_CPUTIME_ID, &now);
		//clock_gettime(CLOCK_MONOTONIC, &now);
		//clock_gettime(CLOCK_MONOTONIC_COARSE, &now);
	}

	gettimeofday(&end, NULL);

	cost = stamp_sub(&end, &begin);
	speed = (n * 1000) / (cost >= 1.0 ? cost : 1.0);
	printf("clock_gettime: total count=%d, cost=%.2f ms, speed=%.2f\r\n",
		n, cost, speed);

	for (i = 0; i < 10; i++) {
		struct timespec now;
		long ms;

		clock_gettime(CLOCK_REALTIME_COARSE, &now);
		ms = now.tv_sec * 1000 + now.tv_nsec / 1000000;
		printf("now: sec=%ld, nsec=%ld, %ld ms\r\n", now.tv_sec, now.tv_nsec, ms);

		clock_gettime(CLOCK_REALTIME, &now);
		ms = now.tv_sec * 1000 + now.tv_nsec / 1000000;
		printf("now: sec=%ld, nsec=%ld, %ld ms\r\n", now.tv_sec, now.tv_nsec, ms);
		printf("-------------------------------------------------\r\n");
	}

	return 0;
}
