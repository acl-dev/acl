#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fiber/libfiber.h"
#include "fiber/libfiber.hpp"

class myfiber : public acl::fiber {
public:
	myfiber(std::vector<acl::fiber_mutex*>& locks) : locks_(locks) {}
	~myfiber(void) {}

private:
	std::vector<acl::fiber_mutex*>& locks_;

	// @override
	void run(void) {
		size_t i = acl::fiber::self() % locks_.size();
		acl::fiber_mutex* lock = locks_[i];

		printf("fiber-%d wait, locks[%zd]=%p\r\n",
			acl::fiber::self(), i, lock);
		lock->lock();
		printf("fiber-%d locked, locks[%ld]=%p\r\n",
			acl::fiber::self(), i, lock);

		sleep(1);

		i = (acl::fiber::self() + 1) % locks_.size();
		lock = locks_[i];

		printf("fiber-%d wait,locks[%zd]=%p\r\n",
			acl::fiber::self(), i, lock);
		lock->lock();
		printf("fiber-%d locked, locks[%zd]=%p\r\n",
			acl::fiber::self(), i, lock);
	}
};

class fiber_check : public acl::fiber {
public:
	fiber_check(void) {}
	~fiber_check(void) {}

private:
	// @override
	void run(void) {
		while (true) {
			sleep(2);
			printf("\r\n");
			acl::fiber_mutex_stats stats;
			if (acl::fiber_mutex::deadlock(stats)) {
				printf("Deadlock happened!\r\n\r\n");
				show(stats);
			} else {
				printf("No deadlock happened!\r\n");
			}
			printf("=======================================\r\n");
		}
	}

	void show(const acl::fiber_mutex_stats& stats) {
		for (std::vector<acl::fiber_mutex_stat>::const_iterator
			cit = stats.stats.begin(); cit != stats.stats.end();
			++cit) {
			show(*cit);
			printf("\r\n");
		}
	}

	void show(const acl::fiber_mutex_stat& stat) {
		printf("fiber-%d:\r\n", acl::fiber::fiber_id(*stat.fb));

		std::vector<acl::fiber_frame> stack;
		acl::fiber::stacktrace(*stat.fb, stack, 50);
		show_stack(stack);

		for (std::vector<ACL_FIBER_MUTEX*>::const_iterator
			cit = stat.holding.begin();
			cit != stat.holding.end(); ++cit) {
			printf("Holding mutex=%p\r\n", *cit);
		}
		printf("Waiting for mutex=%p\r\n", stat.waiting);
	}

	void show_stack(const std::vector<acl::fiber_frame>& stack)
	{
		for (std::vector<acl::fiber_frame>::const_iterator
			cit = stack.begin(); cit != stack.end(); ++cit) {
			printf("0x%lx(%s)+0x%lx\r\n",
				(*cit).pc, (*cit).func.c_str(), (*cit).off);
		}
	}
};

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -c fibers_count -n locks_count\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  i, ch, nlocks = 2, nfibers = 2;
	std::vector<acl::fiber_mutex*> locks;

	while ((ch = getopt(argc, argv, "hc:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			nfibers = atoi(optarg);
			break;
		case 'n':
			nlocks = atoi(optarg);
			if (nlocks <= 0) {
				nlocks = 1;
			}
			break;
		default:
			break;
		}
	}

	for (i = 0; i < nlocks; i++) {
		acl::fiber_mutex* lock = new acl::fiber_mutex;
		locks.push_back(lock);
	}

	std::vector<acl::fiber*> fibers;

	for (i = 0; i < nfibers; i++) {
		acl::fiber* fb = new myfiber(locks);
		fibers.push_back(fb);
		fb->start();
	}

	fiber_check check;
	check.start();

	acl::fiber::schedule();

	for (std::vector<acl::fiber_mutex*>::iterator it = locks.begin();
		it != locks.end(); ++it) {
		delete *it;
	}

	for (std::vector<acl::fiber*>::iterator it = fibers.begin();
		it != fibers.end(); ++it) {
		delete *it;
	}
	return 0;
}
