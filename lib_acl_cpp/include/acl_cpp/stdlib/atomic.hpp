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

class atomic_long_test
{
public:
	atomic_long_test(void) {}
	~atomic_long_test(void) {}

	void run(void)
	{
		atomic_long count;
		long long n = count++;
		printf(">>n=%lld\r\n", n);

		n = count;
		printf(">>n=%lld\r\n", n);

		n = ++count;
		printf(">>n=%lld\r\n", n);

		n = --count;
		printf(">>n=%lld\r\n", n);

		n = count--;
		printf(">>n=%lld\r\n", n);

		n = count;
		printf(">>n=%lld\r\n", n);

		count -= 1;
		n = count;
		printf(">>n=%lld\r\n", n);

		printf(">>count > 1 ? %s\r\n", count >= 1 ? "yes" : "no");
		printf(">>1 > count ? %s\r\n", 1 > count ? "yes" : "no");

		int i = 1;
		count = i;
		n = count;
		printf(">>n=%lld\r\n", n);
	}
};

} // namespace acl
