#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>

static int __delay = 10;
static int __use_lock = 0;
static acl::fiber_event_t __event_type = acl::FIBER_EVENT_T_KERNEL;

class fiber_logger : public acl::fiber
{
public:
	fiber_logger(int nloop, acl::fiber_mutex& lock)
	: nloop_(nloop), lock_(lock) {}

private:
	int nloop_;
	acl::fiber_mutex& lock_;

	~fiber_logger(void) {}

	void run(void)
	{
		for (int i = 0; i < nloop_; i++) {
			if (__use_lock) {
				lock_.lock();
			}

			logger("thread-%lu, fiber-%d, i=%d",
				acl::thread::self(), acl::fiber::self(), i);

			if (__use_lock) {
				lock_.unlock();
			}
		}

		printf("thread-%lu, fiber-%d logger over now!\r\n",
			acl::thread::self(), acl::fiber::self());

		delete this;
	}
};

class thread_logger : public acl::thread
{
public:
	thread_logger(int nfibers, int nloop, acl::fiber_mutex& lock)
	: nfibers_(nfibers)
	, nloop_(nloop)
	, lock_(lock)
	{}

private:
	~thread_logger(void) {}

private:
	int nfibers_;
	int nloop_;
	acl::fiber_mutex& lock_;

	void* run(void)
	{
		for (int i = 0; i < nfibers_; i++) {
			acl::fiber* fb = new fiber_logger(nloop_, lock_);
			fb->start();
		}

		acl::fiber::schedule_with(__event_type);
		return NULL;
	}
};

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -e event_type[kernel|io_uring|poll|select]\r\n"
		" -f logfile\r\n"
		" -t threads_count\r\n"
		" -c fibers_count\r\n"
		" -n nloop[default: 2]\r\n"
		" -d delay[default: 10 ms]\r\n"
		" -L [if use lock when writing log]\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	int  ch, nthreads = 2, nfibers = 2, nloop = 100;
	acl::string logfile("dummy.log");

#define	EQ	!strcasecmp

	while ((ch = getopt(argc, argv, "he:t:c:n:d:L")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'e':
			if (EQ(optarg, "io_uring")) {
				__event_type = acl::FIBER_EVENT_T_IO_URING;
			} else if (EQ(optarg, "poll")) {
				__event_type = acl::FIBER_EVENT_T_POLL;
			} else if (EQ(optarg, "select")) {
				__event_type = acl::FIBER_EVENT_T_SELECT;
			} else if (EQ(optarg, "kernel")) {
				__event_type = acl::FIBER_EVENT_T_KERNEL;
			}
			break;
		case 'f':
			logfile = optarg;
			break;
		case 't':
			nthreads = atoi(optarg);
			break;
		case 'c':
			nfibers = atoi(optarg);
			break;
		case 'n':
			nloop = atoi(optarg);
			break;
		case 'L':
			__use_lock = 1;
			break;
		case 'd':
			__delay = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	//acl::log::stdout_open(true);
	acl::log::open(logfile);
	acl::fiber::stdout_open(true);

	acl::fiber_mutex lock;
	std::vector<acl::thread*> threads;

	for (int i = 0; i < nthreads; i++) {
		acl::thread* thr = new thread_logger(nfibers, nloop, lock);
		threads.push_back(thr);
		thr->start();
	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {
		(*it)->wait();
		delete *it;
	}

	return 0;
}
