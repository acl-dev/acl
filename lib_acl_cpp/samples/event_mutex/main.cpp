#include "stdafx.h"

static int __count = 0;
static bool __show = false;
static int __nloop = 100;

class mythread : public acl::thread
{
public:
	mythread(acl::event_mutex& mutex)
	: mutex_(mutex)
	{
		this->set_detachable(false);
	}

private:
	~mythread(void) {}

private:
	acl::event_mutex& mutex_;

	void* run(void)
	{
		acl_doze(500);

		for (int i = 0; i < __nloop; i++)
		{
			if (mutex_.lock() == false)
			{
				printf("lock error\r\n");
				break;
			}

			__count++;
//			acl_doze(100);
			if (__show)
				printf("thread-%lu locked ok\r\n",
					acl::thread::self());
			assert(mutex_.unlock());
//			acl_doze(10);
		}

		return NULL;
	}
};

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -t nthreads -n loop -S [if show]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int  ch, nthreads = 10;

	while ((ch = getopt(argc, argv, "ht:n:S")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 't':
			nthreads = atoi(optarg);
			break;
		case 'n':
			__nloop = atoi(optarg);
			break;
		case 'S':
			__show = true;
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);
	acl::event_mutex lock(true);
	lock.lock();
	lock.lock();
	lock.unlock();
	lock.unlock();

	printf("test ok, enter any key to continue ...");
	fflush(stdout);
	getchar();

	std::vector<acl::thread*> threads;
	for (int i = 0; i < nthreads; i++)
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

	printf("\r\nAt last, count=%d\r\n", __count);
	return 0;
}
