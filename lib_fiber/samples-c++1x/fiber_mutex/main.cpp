#include "stdafx.h"
#include <vector>
#include <thread>
#include <iostream>

int main() {
	std::vector<std::thread*> threads;
	std::vector<acl::fiber_mutex*> locks;
	std::vector<long long*> cnts;
	size_t number = 2;

	for (size_t i = 0; i < number; i++) {
		auto* lock = new acl::fiber_mutex;
		locks.push_back(lock);
		auto* cnt = new long long;
		*cnt = 0;
		cnts.push_back(cnt);
	}

	acl::fiber::stdout_open(true);

	for (size_t i = 0; i < 10; i++) {
		auto* thr = new std::thread([&] {
			for (size_t j = 0; j < 100; j++) {
				go[&] {
					acl::fiber::delay(5);
					for (size_t m = 0; m < 100; m++) {
						size_t k = m % number;
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

	for (size_t i = 0; i < number; i++) {
		printf("cnts[%zd]=%lld\r\n", i, *cnts[i]);
		delete cnts[i];
		delete locks[i];
	}

	printf("All over!\r\n");
	return 0;
}
