#pragma once
#include "../acl_cpp_define.hpp"

namespace acl {

// internal functions being used
void* atomic_new();
void  atomic_free(void*);
void  atomic_set(void*, void*);
void* atomic_cas(void*, void*, void*);
void* atomic_xchg(void*, void*);

template<typename T>
class ACL_CPP_API atomic {
public:
	atomic(T* t) {
		atomic_ = atomic_new();
		atomic_set(atomic_, t);
	}

	virtual ~atomic() {
		atomic_free(atomic_);
	}

	T* cas(T* cmp, T* val) {
		return (T*) atomic_cas(atomic_, cmp, val);
	}

	T* xchg(T* val) {
		return (T*) atomic_xchg(atomic_, val);
	}

protected:
	void* atomic_;

private:
	atomic(const atomic&);
};

class ACL_CPP_API atomic_long : public atomic<long long> {
public:
	explicit atomic_long(long long n = 0);
	atomic_long(const atomic_long& n);

	~atomic_long() {}

	void set(long long n);
	long long cas(long long cmp, long long n);
	long long fetch_add(long long n);
	long long add_fetch(long long n);

	operator long long() const {
		return n_;
	}

	long long value() const {
		return n_;
	}

	void operator=(long long n) {
		set(n);
	}

	void operator=(const atomic_long& n) {
		set(n.n_);
	}

	long long operator++() {
		return add_fetch(1);
	}

	long long operator++(int) {
		return fetch_add(1);
	}

	long long operator--() {
		return add_fetch(-1);
	}

	long long operator--(int) {
		return fetch_add(-1);
	}

	long long operator+=(long long n) {
		return add_fetch(n);
	}

	long long operator+=(int n) {
		return add_fetch(n);
	}

	long long operator-=(long long n) {
		return add_fetch(-n);
	}

	long long operator-=(int n) {
		return add_fetch(-n);
	}

private:
	long long n_;
};

#if 0
#include "thread.hpp"

class atomic_long_test {
private:
	atomic_long count_;
public:
	atomic_long_test() {}
	~atomic_long_test() {}

	void run() {
		
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

	class mythread : public thread {
	public:
		mythread(atomic_long_test& at) : at_(at) {}
		~mythread() {}
	protected:
		void* run()
		{
			for (size_t i = 0; i < 100; i++)
				at_.run();
			return NULL;
		}
	private:
		atomic_long_test& at_;
	};

	static void test() {
		atomic_long_test at;
		mythread thr1(at), thr2(at), thr3(at);
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
#endif

} // namespace acl
