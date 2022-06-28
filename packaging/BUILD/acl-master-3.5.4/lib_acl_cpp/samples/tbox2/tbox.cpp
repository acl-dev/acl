#include "stdafx.h"

#if 0
 #define TBOX	acl::tbox_array
#else
 #define TBOX	acl::tbox
#endif

class producer : public acl::thread_job
{
public:
	producer(TBOX<int>& box) : box_(box) {}

private:
	~producer(void) {}

protected:
	void* run(void)
	{
		int* n = new int;
		*n = 100;
		box_.push(n, true);

		delete this;
		return NULL;
	}

private:
	TBOX<int>& box_;
};

class consumer : public acl::thread
{
public:
	consumer(acl::thread_pool& threads, int max)
	: threads_(threads), max_(max) {}

protected:
	void* run(void)
	{
		for (int i = 0; i < max_; i++) {
			TBOX<int> box;
			acl::thread_job* job = new producer(box);
			threads_.execute(job);
			int* n = box.pop();
			delete n;
		}

		printf("consumer-%ld finish\r\n", acl::thread::self());
		return NULL;
	}

private:
	acl::thread_pool& threads_;
	int max_;
	~consumer(void) {}
};

int main(void)
{
	acl::thread_pool threads;
	threads.set_limit(500);
	threads.start();

	int nconsumers = 10, max_loop = 1000000;
	std::vector<acl::thread*> consumers;
	for (int i = 0; i < nconsumers; i++) {
		acl::thread* thr = new consumer(threads, max_loop);
		consumers.push_back(thr);
		thr->start();
	}

	for (std::vector<acl::thread*>::iterator it = consumers.begin();
		it != consumers.end(); ++it) {
		(*it)->wait();
		delete *it;
	}

	threads.stop();

	printf("All over: count=%d\r\n", nconsumers * max_loop);
	return 0;
}
