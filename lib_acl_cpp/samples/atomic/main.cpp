#include "acl_cpp/lib_acl.hpp"

double stamp_sub(const struct timeval *from, const struct timeval *sub)
{
	struct timeval res;

	memcpy(&res, from, sizeof(struct timeval));

	res.tv_usec -= sub->tv_usec;
	if (res.tv_usec < 0) {
		--res.tv_sec;
		res.tv_usec += 1000000;
	}
	res.tv_sec -= sub->tv_sec;

	return (res.tv_sec * 1000.0 + res.tv_usec/1000.0);
}

class mythread : public acl::thread
{
public:
	mythread(acl::atomic_long& count, int max)
	: count_(count), max_(max) {}

	~mythread(void) {}

protected:
	void* run(void)
	{
		for (int i = 0; i < max_; i++) {
			count_.add_fetch(1);
			++count_;
		}
		return NULL;
	}

private:
	acl::atomic_long& count_;
	int max_;
};

class mythread2 : public acl::thread
{
public:
	mythread2(acl::locker& lock, long long int& count, int max)
	: lock_(lock)
	, count_(count)
	, max_(max) {}

	~mythread2(void) {}

protected:
	void* run(void)
	{
		for (int i = 0; i < max_; i++) {
			lock_.lock();
			count_++;
			lock_.unlock();
		}

		return NULL;
	}

private:
	acl::locker& lock_;
	long long int& count_;
	int  max_;
};

static void test(void)
{
	acl::atomic_long n1(10), n2;

	printf("n1=%lld, n2=%lld\r\n", n1.value(), n2.value());
	n2 = n1;
	printf("n1=%lld, n2=%lld\r\n", n1.value(), n2.value());
}

int main(void)
{
	test();
	acl::atomic_long_test test;
	test.run();

	//////////////////////////////////////////////////////////////////////

	printf("-------------------------------------------------------\r\n");

	long long n;
	acl::atomic_long count(0);

	n = count.fetch_add(100);
	printf(">>n: %lld\r\n", n);

	n = count.add_fetch(200);
	printf(">>n: %lld\r\n", n);

	std::vector<acl::thread*> threads;
	int max = 1000000;

	struct timeval begin, end;
	gettimeofday(&begin, NULL);

	for (int i = 0; i < 20; i++) {
		acl::thread* thr = new mythread(count, max);
		thr->set_detachable(false);
		threads.push_back(thr);
		thr->start();
	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {
		(*it)->wait();
		delete *it;
	}

	gettimeofday(&end, NULL);

	n = count--;
	count += 100;

	if (100 < count && count >= 1)
		printf(">>n: %lld, %lld, spent: %.2f\r\n",
			count.value(), n, stamp_sub(&end, &begin));

	//////////////////////////////////////////////////////////////////////

	acl::locker lock;
	n = 0;

	threads.clear();

	for (int i = 0; i < 20; i++) {
		acl::thread* thr = new mythread2(lock, n, max);
		thr->set_detachable(false);
		threads.push_back(thr);
		thr->start();
	}

	gettimeofday(&begin, NULL);

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {
		(*it)->wait();
		delete *it;
	}
	gettimeofday(&end, NULL);

	printf(">>n: %lld, spent: %.2f\r\n", n, stamp_sub(&end, &begin));
	return 0;
}
