#include "stdafx.h"
#include <vector>
#include <thread>
#include <iostream>
#include <getopt.h>

static void usage(const char *proc) {
	printf("usage: %s -h[help]\r\n"
		"    -t threads_count[default: 2]\r\n"
		"    -c fibers_count_per_thread[default: 10]\r\n"
		"    -n loop_count_per_fiber[default: 100]\r\n"
		"    -l locks_count[default: 2]\r\n", proc);
}

int main(int argc, char *argv[]) {
	std::vector<std::thread*> threads;
	std::vector<acl::fiber_mutex*> locks;
	std::vector<long long*> cnts;
	int number = 2, nthreads = 2, nfibers = 10, count = 100;;
	int ch;

	while ((ch = getopt(argc, argv, "ht:c:n:l:")) > 0) {
		switch (ch) {
		case 'h':
		default:
			usage(argv[0]);
			return 0;
		case 't':
			nthreads = std::atoi(optarg);
			break;
		case 'c':
			nfibers = std::atoi(optarg);
			break;
		case 'n':
			count = std::atoi(optarg);
			break;
		case 'l':
			number = std::atoi(optarg);
			break;
		}
	}

	for (int i = 0; i < number; i++) {
		auto* lock = new acl::fiber_mutex;
		locks.push_back(lock);
		auto* cnt = new long long;
		*cnt = 0;
		cnts.push_back(cnt);
	}

	acl::fiber::stdout_open(true);

	for (int i = 0; i < nthreads; i++) {
		auto* thr = new std::thread([&] {
			for (int j = 0; j < nfibers; j++) {
				go[&] {
					acl::fiber::delay(5);
					for (int m = 0; m < count; m++) {
						int k = m % number;
						locks[k]->lock();
						*cnts[k] += 1;
						acl::fiber::yield();
						locks[k]->unlock();
					}
				};
			}

			acl::fiber::schedule();
		});
		threads.push_back(thr);
	}

	for (auto it : threads) {
		it->join();
		delete it;
	}

	for (int i = 0; i < number; i++) {
		printf("cnts[%d]=%lld\r\n", i, *cnts[i]);
		delete cnts[i];
		delete locks[i];
	}

	printf("All over!\r\n");
	return 0;
}
