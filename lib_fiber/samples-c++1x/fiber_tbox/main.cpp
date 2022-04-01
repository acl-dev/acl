#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include "../stamp.h"

static void box_wakeup(acl::fiber_tbox<bool>& box)
{
	box.push(NULL);
}

static void box_wait(acl::fiber_tbox<bool>& box)
{
	box.pop();
}

static void test_in_shared_stack(size_t cocurrent, size_t n, size_t& count,
	bool interactive)
{
	go[&] {
		size_t total = cocurrent * n;
		acl::fiber_tbox<bool>* box = new acl::fiber_tbox<bool>;
		acl::fiber_tbox<bool>* box1;
		if (interactive) {
			box1 = new acl::fiber_tbox<bool>;
		} else {
			box1 = NULL;
		}

		for (size_t j = 0; j < cocurrent; j++) {
			go[=] {
				for (size_t i = 0; i < n; i++) {
					if (box1) {
						box_wait(*box1);
					}

					box_wakeup(*box);
				}
			};
		}

		for (size_t i = 0; i < total; i++) {
			if (box1) {
				box_wakeup(*box1);
			}

			box_wait(*box);
			count++;
		}

		delete box;
		delete box1;
		printf(">>> wait over, count=%zd\r\n", count);
	};
}

static void test_in_shared_stack_thread(size_t cocurrent, size_t n,
	size_t& count, bool interactive)
{
	go[&] {
		size_t total = cocurrent * n;
		acl::fiber_tbox<bool>* box = new acl::fiber_tbox<bool>;
		acl::fiber_tbox<bool>* box1;

		if (interactive) {
			box1 = new acl::fiber_tbox<bool>;
		} else {
			box1 = NULL;
		}

		for (size_t j = 0; j < cocurrent; j++) {
			std::thread thread([=] {
				for (size_t i = 0; i < n; i++) {
					if (box1) {
						box_wait(*box1);
					}

					box_wakeup(*box);
				}
			});
			thread.detach();
		}

		for (size_t i = 0; i < total; i++) {
			if (box1) {
				box_wakeup(*box1);
			}

			box_wait(*box);
			count++;
		}

		delete box;
		delete box1;

		printf(">>> wait over, count=%zd\r\n", count);
	};
}

static void test_in_private_stack(size_t cocurrent, size_t n, size_t& count,
		bool interactive)
{
	size_t total = cocurrent * n;

	go[&] {
		acl::fiber_tbox<bool> box;
		acl::fiber_tbox<bool> box1;

		for (size_t j = 0; j < cocurrent; j++) {
			go[&] {
				for (size_t i = 0; i < n; i++) {
					if (interactive) {
						box_wait(box1);
					}

					box_wakeup(box);
				}
			};
		}

		for (size_t i = 0; i < total; i++) {
			if (interactive) {
				box_wakeup(box1);
			}

			box_wait(box);
			count++;
		}

		printf(">>> wait over, count=%zd\r\n", count);
	};
}

static void test_in_private_stack_thread(size_t cocurrent, size_t n,
	size_t& count, bool interactive)
{
	go[&] {
		size_t total = cocurrent * n;
		acl::fiber_tbox<bool> box;
		acl::fiber_tbox<bool> box1;

		for (size_t j = 0; j < cocurrent; j++) {
			std::thread thread([&] {
				for (size_t i = 0; i < n; i++) {
					if (interactive) {
						box_wait(box1);
					}

					box_wakeup(box);
				}
			});
			thread.detach();
		}

		for (size_t i = 0; i < total; i++) {
			if (interactive) {
				box_wakeup(box1);
			}

			box_wait(box);
			count++;
		}

		printf(">>> wait over, count=%zd\r\n", count);
	};
}
static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -c cocurrent\r\n"
		" -n loop_count\r\n"
		" -T [if use_thread]\r\n"
		" -B [if interactive]\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	int  ch, n = 1, cocurrent = 1;
	bool use_shared_stack = false, use_thread = false, interactive = false;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hc:n:STB")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			cocurrent = atoi(optarg);
			break;
		case 'n':
			n = atoi(optarg);
			break;
		case 'S':
			use_shared_stack = true;
			break;
		case 'T':
			use_thread = true;
			break;
		case 'B':
			interactive = true;
			break;
		default:
			break;
		}
	}

	size_t shared_stack_size = acl::fiber::get_shared_stack_size();

	printf(">>> shared_stack_size: %zd\r\n", shared_stack_size);

	struct timeval begin;
	gettimeofday(&begin, NULL);

	size_t count = 0;

	if (use_shared_stack > 0) {
		if (use_thread) {
			test_in_shared_stack_thread(cocurrent, n, count,
				interactive);
		} else {
			test_in_shared_stack(cocurrent, n, count, interactive);
		}
	} else {
		if (shared_stack_size > 0) {
			printf(">>> This will be crashed in shared stack!\r\n");
		}

		if (use_thread) {
			test_in_private_stack_thread(cocurrent, n, count,
				interactive);
		} else {
			test_in_private_stack(cocurrent, n, count, interactive);
		}
	}

	acl::fiber::schedule();

	struct timeval end;
	gettimeofday(&end, NULL);

	double cost = stamp_sub(&end, &begin);
	double speed = (count * 1000) / (cost > 0 ? cost : 0.1);
	printf(">>> Total count=%zd, cost=%.2f ms, speed=%.2f qps\r\n",
		count, cost, speed);
	return 0;
}
