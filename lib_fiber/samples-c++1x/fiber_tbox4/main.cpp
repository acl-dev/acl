#include "stdafx.h"
#include <getopt.h>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <memory>
#include <atomic>

static void consuming(acl::fiber_tbox<int>& box, int max) {
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

static void test1(int max, bool in_fiber, bool blocking, bool notify_first) {
	std::shared_ptr<acl::fiber_tbox<int>> box(new
		acl::fiber_tbox<int>(true, !blocking));

	std::shared_ptr<acl::wait_group> wg(new acl::wait_group());

	wg->add(1);
	std::thread([box, wg, max, in_fiber] {
		if (in_fiber) {
			go[box, max] {
				consuming(*box, max);
			};
			acl::fiber::schedule();
		} else {
			consuming(*box, max);
		}

		wg->done();
	}).detach();

	go[box, wg, max, notify_first] {
		int i;
		for (i = 0; i < max; i++) {
			int *n = new int(i);
			box->push(n, notify_first);
#if 0
			if (i > 0 && i % 1000 == 0) {
				char buf[128];
				snprintf(buf, sizeof(buf), "i=%d", i);
				acl::meter_time(__FILE__, __LINE__, buf);
			}
#endif
		}

		printf("Push over, count=%d, max=%d\r\n", i, max);
		wg->wait();
	};

	acl::fiber::schedule();
}

static void usage(const char *procname) {
	printf("usage: %s -h[help]\r\n"
		" -n max\r\n"
		" -B [if use locker in blocking mode]\r\n"
		" -F [if consumer in fiber]\r\n"
		" -S [if notify first]\r\n",
		procname);
}

int main(int argc, char *argv[]) {
	int ch, max = 1000;
	bool in_fiber = false, blocking = false, notify_first = false;

	while ((ch = getopt(argc, argv, "hn:FBS")) > 0) {
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
	test1(max, in_fiber, blocking, notify_first);
	gettimeofday(&end, nullptr);

	printf("All over now, max=%d, cost=%.2f\r\n",
		max, acl::stamp_sub(end, begin));
	return 0;
}
