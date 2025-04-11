#include "stdafx.h"
#include <getopt.h>
#include <cstdio>
#include <cstdlib>
#include <thread>

static void consuming(acl::fiber_mutex& mutex, acl::fiber_cond& cond, int max) {
	int i;
	mutex.lock();
	for (i = 0; i < max; i++) {
		if (!cond.wait(mutex, 100)) {
			printf("Wait timeout!\n");
			break;
		}
	}
	mutex.unlock();
	printf("Consumer over, i=%d, max=%d\n", i, max);
}

static int test(int max, bool in_fiber) {
	acl::wait_group wg;
	acl::fiber_mutex mutex;
	acl::fiber_cond cond;

	wg.add(1);
	std::thread([in_fiber, &wg, &cond, &mutex, max] {
		if (in_fiber) {
			go[&wg, &cond, &mutex, max] {
				consuming(mutex, cond, max);
			};
			acl::fiber::schedule();
		} else {
			consuming(mutex, cond, max);
		}
		wg.done();
	}).detach();

	go[&wg, &cond, &mutex, max] {
		for (int i = 0; i < max; i++) {
			mutex.lock();
			cond.notify();
			mutex.unlock();
			//acl::fiber::delay(1);
			//acl::fiber::yield();
		}

		printf("Producer over now, count=%d!\n", max);
		wg.wait();
	};


	acl::fiber::schedule();

	return 0;
}

static void usage(const char* procname) {
	printf("usage: %s -h[help] -n max -F [if Consumer in fiber mode]\r\n", procname);
}

int main(int argc, char *argv[]) {
	int max = 1000, ch;
	bool in_fiber = false;

	while ((ch = getopt(argc, argv, "hn:F")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			max = atoi(optarg);
			break;
		case 'F':
			in_fiber = true;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	struct timeval begin;
	gettimeofday(&begin, nullptr);
	test(max, in_fiber);
	struct timeval end;
	gettimeofday(&end, nullptr);
	printf("Total cost: %.2f\r\n", acl::stamp_sub(end, begin));
	return 0;
}
