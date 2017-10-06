#include "stdafx.h"

static int __count = 0;

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

		for (int i = 0; i < 100; i++)
		{
			if (mutex_.lock() == false)
			{
				printf("lock error\r\n");
				break;
			}

			__count++;
//			acl_doze(100);
			printf("thread-%lu locked ok\r\n", acl::thread::self());
			assert(mutex_.unlock());
			acl_doze(10);
		}

		return NULL;
	}
};

int main(void)
{
	acl::event_mutex lock;
	lock.lock();
	lock.lock();
	lock.unlock();
	lock.unlock();

	printf("test ok, enter any key to continue ...");
	fflush(stdout);
	getchar();

	std::vector<acl::thread*> threads;
	for (int i = 0; i < 10; i++)
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
