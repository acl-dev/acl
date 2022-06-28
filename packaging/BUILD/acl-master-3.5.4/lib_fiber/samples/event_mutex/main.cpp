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

class myfiber : public acl::fiber
{
public:
	myfiber(acl::event_mutex& lock, int& nfibers)
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

			if (lock_.lock() == false)
			{
				printf("lock error %s\r\n", acl::last_serror());
				break;
			}

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

			lock_.unlock();
			__counter++;

			if (__show)
				printf("thread-%lu, fiber-%u unlock ok, counter-%lld\r\n",
					acl::thread::thread_self(), acl::fiber::self(),
					__counter.value());
//			acl_doze(10);
//			acl::fiber::yield();
		}

		--nfibers_;
		printf("thread-%lu, nfibers: %d\r\n", acl::thread::self(), nfibers_);
		if (nfibers_ == 0)
			acl::fiber::schedule_stop();

		delete this;
	}

private:
	acl::event_mutex& lock_;
	int& nfibers_;

	~myfiber(void) {}
};

class mythread : public acl::thread
{
public:
	mythread(acl::event_mutex& lock) : lock_(lock)
	{
		this->set_detachable(false);
	}
	~mythread(void) {}

private:
	acl::event_mutex& lock_;

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
		" -S [show info]\r\n"
		" -d delay[default: 100 ms]\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	int  ch;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hc:t:n:d:S")) > 0)
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
		case 'd':
			__delay = atoi(optarg);
			break;
		case 'S':
			__show = true;
			break;
		default:
			break;
		}
	}


	acl::event_mutex lock(false);

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

	printf("all over, thread=%d, fibers=%d, nloop=%d, counter=%lld\r\n",
		__nthreads, __nfibers, __nloop, __counter.value());
	return 0;
}
