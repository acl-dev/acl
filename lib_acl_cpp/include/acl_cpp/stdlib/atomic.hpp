#pragma once
#include "../acl_cpp_define.hpp"

struct ACL_ATOMIC;

namespace acl
{

template<typename T>
class atomic
{
public:
	atomic(T* t);

	virtual ~atomic(void);

	T* cas(T* cmp, T* val);
	T* xchg(T* val);

protected:
	ACL_ATOMIC* atomic_;
};

class atomic_long : public atomic<long long>
{
public:
	atomic_long(long long n = 0);

	~atomic_long(void) {}

	void set(long long n);
	long long fetch_add(long long n);
	long long add_fetch(long long n);

	operator long long() const
	{
		return n_;
	}

	long long value(void)
	{
		return n_;
	}

	void operator=(long long n)
	{
		set(n);
	}

	long long operator++()
	{
		return add_fetch(1);
	}

	long long operator++(int)
	{
		return fetch_add(1);
	}

	long long operator--()
	{
		return add_fetch(-1);
	}

	long long operator--(int)
	{
		return fetch_add(-1);
	}

	long long operator+=(long long n)
	{
		return add_fetch(n);
	}

	long long operator+=(int n)
	{
		return add_fetch(n);
	}

	long long operator-=(long long n)
	{
		return add_fetch(-n);
	}

	long long operator-=(int n)
	{
		return add_fetch(-n);
	}

private:
	long long n_;
};

#include "thread.hpp"

class atomic_long_test
{
private:
	atomic_long count_;
public:
	atomic_long_test(void) {}
	~atomic_long_test(void) {}

	void run(void)
	{
		
		long long n = count_++;
		printf(">>n=%lld\r\n", n);

		n = count_;
		printf(">>n=%lld\r\n", n);

		n = ++count_;
		printf(">>n=%lld\r\n", n);

		n = --count_;
		printf(">>n=%lld\r\n", n);

		n = count_--;
		printf(">>n=%lld\r\n", n);

		n = count_;
		printf(">>n=%lld\r\n", n);

		count_ -= 1;
		n = count_;
		printf(">>n=%lld\r\n", n);

		printf(">>count > 1 ? %s\r\n", count_ >= 1 ? "yes" : "no");
		printf(">>1 > count ? %s\r\n", 1 > count_ ? "yes" : "no");

		int i = 1;
		count_ = i;
		n = count_;
		printf(">>n=%lld\r\n", n);
	}

	static void test(void)
	{
		class mythread : public thread
		{
		public:
			mythread(atomic_long_test& alt) : alt_(alt) {}
			~mythread(void) {}
		protected:
			void* run(void)
			{
				for (size_t i = 0; i < 100; i++)
					alt_.run();
				return NULL;
			}
		private:
			atomic_long_test& alt_;
		};

		atomic_long_test alt;
		mythread thr1(alt), thr2(alt), thr3(alt);
		thr1.set_detachable(false);
		thr2.set_detachable(false);
		thr3.set_detachable(false);
		thr1.start();
		thr2.start();
		thr3.start();
		thr1.wait();
		thr2.wait();
		thr3.wait();
	}
};

} // namespace acl
