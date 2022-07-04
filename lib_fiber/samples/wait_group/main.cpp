#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include "wait_group.h"

static int __delay = 1;

//////////////////////////////////////////////////////////////////////////////

class fiber_notifier : public acl::fiber
{
public:
	fiber_notifier(wait_group& sync) : sync_(sync) {}

private:
	~fiber_notifier(void) {}

private:
	wait_group& sync_;

	// @override
	void run(void) {
		printf("Fiber-%u started\r\n", acl::fiber::self());
		sleep(__delay);

		printf("Fiber-%u done\r\n", acl::fiber::self());
		sync_.done();

		delete this;
	}
};

//////////////////////////////////////////////////////////////////////////////

class thread_notifier : public acl::thread
{
public:
	thread_notifier(wait_group& sync) : sync_(sync) {}

private:
	~thread_notifier(void) {}

private:
	wait_group& sync_;

	// @override
	void* run(void) {
		printf("Thread-%lu started\r\n", acl::thread::self());
		sleep(__delay);

		printf("Thread-%lu done\r\n", acl::thread::self());
		sync_.done();

		delete this;
		return NULL;
	}
};

//////////////////////////////////////////////////////////////////////////////

class fiber_waiter : public acl::fiber
{
public:
	fiber_waiter(wait_group& sync) : sync_(sync) {}
	~fiber_waiter(void) {}

private:
	wait_group& sync_;

	// @override
	void run(void) {
		size_t ret = sync_.wait();
		printf("All threads and fibers were done, ret=%zd\r\n", ret);
	}
};

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -t threads_count[default: 1]\r\n"
		" -c fibers_count[default: 1]\r\n"
		" -d delay[default: 1 ms]\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	int  ch, threads_count = 1, fibers_count = 1;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "t:f:c:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 't':
			threads_count = atoi(optarg);
			break;
		case 'c':
			fibers_count = atoi(optarg);
			break;
		case 'd':
			__delay = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);
	acl::fiber::stdout_open(true);

	wait_group sync;
	sync.add(threads_count + fibers_count);

	for (int i = 0; i < threads_count; i++) {
		acl::thread* thr = new thread_notifier(sync);
		thr->set_detachable(true);
		thr->start();
	}

	for (int i = 0; i < fibers_count; i++) {
		acl::fiber* fb = new fiber_notifier(sync);
		fb->start();
	}

	fiber_waiter waiter(sync);
	waiter.start();

	acl::fiber::schedule();

	return 0;
}
