#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>

static int __nthreads = 2;
static int __nloop    = 2;
static int __delay    = 0;

static acl::atomic_long __counter = 0;
static long long __counter2 = 0;

class mythread : public acl::thread
{
public:
	mythread(acl::fiber_event& event) : event_(event)
	{
		this->set_detachable(false);
	}

private:
	~mythread(void) {}

private:
	acl::fiber_event& event_;

	// @override
	void* run(void)
	{
		for (int i = 0; i < __nloop; i++)
		{
			assert(event_.wait());
			if (__delay > 0)
				acl_doze(__delay);
			__counter2++;
			assert(event_.notify());
			__counter++;
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
		" -d delay[default: 100 ms]\r\n",
		procname);
}

int main(int argc, char *argv[])
{
	int  ch;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "ht:n:d:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 't':
			__nthreads = atoi(optarg);
			break;
		case 'n':
			__nloop = atoi(optarg);
			break;
		case 'd':
			__delay = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::fiber_event event;

	std::vector<acl::thread*> threads;
	for (int i = 0; i < __nthreads; i++)
	{
		acl::thread* thr = new mythread(event);
		threads.push_back(thr);
		thr->start();
	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it)
	{
		(*it)->wait();
		delete *it;
	}

	printf("all over, thread=%d, nloop=%d, counter=%lld, %lld\r\n",
		__nthreads, __nloop, __counter.value(), __counter2);

	return 0;
}
