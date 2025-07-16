#include "stdafx.h"
#include <thread>

int main() {
	acl::fiber::stdout_open(true);

	acl::fiber_mutex lock1, lock2;

	std::thread thr1([&] {
		go[&] {
			lock1.lock();
			printf(">>>thread=%ld, fiber=%d locked lock1\n", (long) pthread_self(), acl::fiber::self());
			acl::fiber::delay(2000);

			printf(">>>thread=%ld, fiber=%d unlock lock1\n", (long) pthread_self(), acl::fiber::self());
			lock1.unlock();
			printf(">>>thread=%ld, fiber=%d unlocked lock1\n", (long) pthread_self(), acl::fiber::self());
		};

		go[&] {
			lock2.lock();
			printf(">>>thread=%ld, fiber=%d locked lock2\n", (long) pthread_self(), acl::fiber::self());
			acl::fiber::delay(2000);

			printf(">>>thread=%ld, fiber=%d unlock lock2\n", (long) pthread_self(), acl::fiber::self());
			lock2.unlock();
			printf(">>>thread=%ld, fiber=%d unlocked lock2\n", (long) pthread_self(), acl::fiber::self());
		};

		acl::fiber::schedule();
	});

	::sleep(1);

	std::thread thr2([&] {
		go[&] {
			printf(">>>thread=%ld, fiber=%d lock lock1\n", (long) pthread_self(), acl::fiber::self());
			lock1.lock();
			printf(">>>thread=%ld, fiber=%d locked lock1\n", (long) pthread_self(), acl::fiber::self());
			lock1.unlock();
		};

		go[&] {
			printf(">>>thread=%ld, fiber=%d lock lock2\n", (long) pthread_self(), acl::fiber::self());
			lock2.lock();
			printf(">>>thread=%ld, fiber=%d locked lock2\n", (long) pthread_self(), acl::fiber::self());
			lock2.unlock();
		};

		acl::fiber::schedule();
	});


	thr1.join();
	thr2.join();

	printf("All over!\r\n");
	return 0;
}
