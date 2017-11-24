#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>

static bool __show    = false;
static int __nthreads = 2;
static int __nfibers  = 2;
static int __nloop    = 2;
static int __delay    = 0;

static acl::atomic_long __counter = 0;
static long long __counter2 = 0;

class myfiber : public acl::fiber
{
public:
	myfiber(acl::fiber_event& lock, int& nfibers)
	: lock_(lock)
	, nfibers_(nfibers)
	{
	}

protected:
	// @override
	void run(void)
	{
		for (int i = 0; i < __nloop; i++)
		{
			if (__show)
				printf("thread-%lu, fiber-%u begin lock\r\n",
					acl::thread::thread_self(),
					acl::fiber::self());
			assert(lock_.wait());

			if (__show)
				printf("thread-%lu, fiber-%u lock ok\r\n",
					acl::thread::thread_self(),
					acl::fiber::self());

			if (__delay > 0)
				acl_doze(__delay);

			if (__show)
				printf("thread-%lu, fiber-%u begin unlock\r\n",
					acl::thread::thread_self(),
					acl::fiber::self());

			__counter2++;
			assert(lock_.notify());
			__counter++;

			if (__show)
				printf("thread-%lu, fiber-%u unlock ok, counter-%lld\r\n",
					acl::thread::thread_self(),
					acl::fiber::self(),
					__counter.value());
		}

		--nfibers_;
		if (nfibers_ == 0)
			acl::fiber::schedule_stop();

		delete this;
	}

private:
	acl::fiber_event& lock_;
	int& nfibers_;

	~myfiber(void) {}
};

class mythread : public acl::thread
{
public:
	mythread(acl::fiber_event& lock) : lock_(lock)
	{
		this->set_detachable(false);
	}
	~mythread(void) {}

private:
	acl::fiber_event& lock_;

	// @override
	void* run(void)
	{
		int nfibers = __nfibers, n = nfibers;

		for (int i = 0; i < n; i++)
		{
			acl::fiber* fb = new myfiber(lock_, nfibers);
			fb->start();
		}

		acl::fiber::schedule();
		return NULL;
	}
};

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -t nthreads[default: 2]\r\n"
		" -c nfibers[default: 2]\r\n"
		" -n nloop[default: 2]\r\n"
		" -d delay[default: 0 ms]\r\n"
		" -S [show info]\r\n",
		procname);
}

int main(int argc, char *argv[])
{
	int  ch;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hc:t:n:Sd:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			__nfibers = atoi(optarg);
			break;
		case 't':
			__nthreads = atoi(optarg);
			break;
		case 'n':
			__nloop = atoi(optarg);
			break;
		case 'S':
			__show = true;
			break;
		case 'd':
			__delay = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::fiber_event lock;

	std::vector<acl::thread*> threads;
	for (int i = 0; i < __nthreads; i++)
	{
		acl::thread* thr = new mythread(lock);
		threads.push_back(thr);
		thr->start();
	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it)
	{
		(*it)->wait();
		delete *it;
	}

	printf("all over, thread=%d, fibers=%d, nloop=%d, counter=%lld, %lld\n",
		__nthreads, __nfibers, __nloop, __counter.value(), __counter2);
	return 0;
}
