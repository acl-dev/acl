#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>

static int __nloop    = 2;
static int __delay    = 10;

static acl::atomic_long __producing = 0;
static acl::atomic_long __consuming = 0;

class producer : public acl::thread
{
public:
	producer(acl::fiber_event& event, acl::fiber_cond& cond)
	: event_(event)
	, cond_(cond)
	{
		this->set_detachable(false);
	}

private:
	~producer(void) {}

private:
	acl::fiber_event& event_;
	acl::fiber_cond&  cond_;

	// @override
	void* run(void)
	{
		for (int i = 0; i < __nloop; i++) {
			if (__delay > 0) {
				acl_doze(__delay);
			}
			printf(">> producer(%ld): lock\r\n", this->self());
			assert(event_.wait());
			printf(">> producer(%ld): notify\r\n", this->self());
			assert(cond_.notify());
			printf(">> producer(%ld): unlock\r\n", this->self());
			assert(event_.notify());
			__producing++;
		}

		return NULL;
	}
};

class consumer : public acl::thread
{
public:
	consumer(acl::fiber_event& event, acl::fiber_cond& cond,
		long long timeout)
	: event_(event)
	, cond_(cond)
	, timeout_(timeout)
	{
		this->set_detachable(false);
	}

private:
	~consumer(void) {}

private:
	acl::fiber_event& event_;
	acl::fiber_cond&  cond_;
	long long         timeout_;

	void* run(void)
	{
		printf("<< consumer: lock\r\n");
		assert(event_.wait());
		for (int i = 0; i < __nloop; i++) {
			printf("<< consumer(%ld): wait\r\n", this->self());
			if (cond_.wait(event_, timeout_) == false) {
				printf("<< consumer: wait %s\r\n", 
					acl::fiber::last_serror());
				break;
			}
			printf("<< consumer(%ld): wakeup\r\n", this->self());
			__consuming++;
		}

		return NULL;
	}
};

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -t nthreads[default: 2]\r\n"
		" -n nloop[default: 2]\r\n"
		" -d delay[default: 0 ms]\r\n"
		" -t wait_timeout[default: -1]\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	int  ch, nproducer = 1, nconsumer = 1, timeout = -1;

	bool use_mutex = false;
	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hp:c:n:d:mt:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'p':
			nproducer = atoi(optarg);
			break;
		case 'c':
			nconsumer = atoi(optarg);
			break;
		case 'n':
			__nloop = atoi(optarg);
			break;
		case 'd':
			__delay = atoi(optarg);
			break;
		case 'm':
			use_mutex = true;
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);
	acl::fiber::stdout_open(true);
	acl::fiber_event event(use_mutex);
	acl::fiber_cond  cond;

	std::vector<acl::thread*> threads;
	for (int i = 0; i < nconsumer; i++) {
		acl::thread* thr = new consumer(event, cond, timeout);
		threads.push_back(thr);
		thr->start();
	}

	for (int i = 0; i < nproducer; i++) {
		acl::thread* thr = new producer(event, cond);
		threads.push_back(thr);
		thr->start();
	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {

		(*it)->wait();
		delete *it;
	}

	printf("all over, nloop=%d, producing=%lld, consuming=%lld\r\n",
		__nloop, __producing.value(), __consuming.value());

	return 0;
}
