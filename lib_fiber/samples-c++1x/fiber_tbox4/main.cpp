#include "stdafx.h"
#include <getopt.h>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <memory>
#include <atomic>

static void consuming1(acl::fiber_tbox<int>& box, int max) {
	int i = 0, cnt = 0;
	for (i = 0; i < max; i++) {
		int *n = box.pop();
		if (n == nullptr) {
			printf("POP error\r\n");
			break;
		}
		//printf(">>>pop one n: %d\n", *n);
		delete n;
		cnt++;
	}

	printf("The last n is %d, count is %d\r\n", i, cnt);
}

static void test1(int max, bool read_in_fiber, bool send_in_fiber,
		bool blocking, bool notify_first) {
	std::shared_ptr<acl::fiber_tbox<int>> box(new
		acl::fiber_tbox<int>(true, !blocking));

	std::shared_ptr<acl::wait_group> wg(new acl::wait_group());

	wg->add(1);
	std::thread([box, wg, max, read_in_fiber] {
		if (read_in_fiber) {
			go[box, max] {
				consuming1(*box, max);
			};
			acl::fiber::schedule();
		} else {
			consuming1(*box, max);
		}

		wg->done();
	}).detach();

	if (!send_in_fiber) {
		int i;
		for (i = 0; i < max; i++) {
			int *n = new int(i);
			box->push(n, notify_first);
		}

		printf("Push over, count=%d, max=%d\r\n", i, max);
		wg->wait();
		return;
	}

	go[box, wg, max, notify_first] {
		int i;
		for (i = 0; i < max; i++) {
			int *n = new int(i);
			box->push(n, notify_first);
		}

		printf("Push over, count=%d, max=%d\r\n", i, max);
		wg->wait();
	};

	acl::fiber::schedule();
}

//////////////////////////////////////////////////////////////////////////////

static void consuming2(acl::fiber_tbox2<int>& box, int max) {
	int i = 0, cnt = 0;
	for (i = 0; i < max; i++) {
		int n;
		if (!box.pop(n)) {
			printf("POP error\r\n");
			break;
		}
		//printf(">>>pop one n: %d\n", i);
		cnt++;
	}

	printf("The last n is %d, count is %d\r\n", i, cnt);
}

static void test2(int max, bool read_in_fiber, bool send_in_fiber,
		bool blocking, bool notify_first) {
	std::shared_ptr<acl::fiber_tbox2<int>> box(new acl::fiber_tbox2<int>(!blocking));

	std::shared_ptr<acl::wait_group> wg(new acl::wait_group());

	wg->add(1);
	std::thread([box, wg, max, read_in_fiber] {
		if (read_in_fiber) {
			go[box, max] {
				consuming2(*box, max);
			};
			acl::fiber::schedule();
		} else {
			consuming2(*box, max);
		}

		wg->done();
	}).detach();

	if (!send_in_fiber) {
		int i;
		for (i = 0; i < max; i++) {
			box->push(i, notify_first);
		}

		printf("Push over, count=%d, max=%d\r\n", i, max);
		wg->wait();
		return;
	}

	go[box, wg, max, notify_first] {
		int i;
		for (i = 0; i < max; i++) {
			box->push(i, notify_first);
		}

		printf("Push over, count=%d, max=%d\r\n", i, max);
		wg->wait();
	};

	acl::fiber::schedule();
}

//////////////////////////////////////////////////////////////////////////////

static void usage(const char *procname) {
	printf("usage: %s -h[help]\r\n"
		" -n max\r\n"
		" -B [if use locker in blocking mode]\r\n"
		" -C [if consumer in fiber]\r\n"
		" -P [if producer in fiber]\r\n"
		" -S [if notify first]\r\n",
		procname);
}

int main(int argc, char *argv[]) {
	int ch, max = 1000;
	bool read_in_fiber = false, send_in_fiber = false;
	bool blocking = false, notify_first = false;

	while ((ch = getopt(argc, argv, "hn:CPBS")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			max = atoi(optarg);
			break;
		case 'C':
			read_in_fiber = true;
			break;
		case 'P':
			send_in_fiber = true;
			break;
		case 'B':
			blocking = true;
			break;
		case 'S':
			notify_first = true;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	struct timeval begin, end;
	gettimeofday(&begin, nullptr);
	test1(max, read_in_fiber, send_in_fiber, blocking, notify_first);
	gettimeofday(&end, nullptr);
	printf("fiber_tbox over now, max=%d, cost=%.2f\r\n", max, acl::stamp_sub(end, begin));

	gettimeofday(&begin, nullptr);
	test2(max, read_in_fiber, send_in_fiber, blocking, notify_first);
	gettimeofday(&end, nullptr);
	printf("fiber_tbox2 over now, max=%d, cost=%.2f\r\n", max, acl::stamp_sub(end, begin));

	return 0;
}
