#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include <stdlib.h>
#include "thread_mutex.hpp"
#include "thread_cond.hpp"
#include "box.hpp"

namespace acl {

/**
 * Used for message communication between threads, implemented through thread
 * condition variables and thread locks
 *
 * Example:
 *
 * class myobj {
 * public:
 *     myobj(void) {}
 *     ~myobj(void) {}
 *
 *     void test(void) { printf("hello world\r\n"); }
 * };
 *
 * acl::tbox2<myobj> tbox;
 *
 * void thread_producer() {
 *     myobj o;
 *     tbox.push(o);
 * }
 *
 * void thread_consumer() {
 *     myobj o;
 *     if (tbox.pop(o)) {
 *         o->test();
 *     }
 * }
 */

template<typename T>
class tbox2  : public box2<T> {
public:
	/**
	 * Constructor
	 */
	tbox2() : size_(0), cond_(&lock_) {}

	~tbox2() {}

	/**
	 * Clear unconsumed message objects in the message queue
	 */
	void clear() {
		box_.clear();
	}

	/**
	 * Send message object
	 * @param t {T} Message object
	 * @param notify_first {bool} If true, notify first then unlock, otherwise
	 * unlock first
	 *  then notify. Note the difference between the two
	 * @return {bool}
	 * @override
	 */
	bool push(T t, bool notify_first = true) {
		if (! lock_.lock()) { abort(); }

#if __cplusplus >= 201103L || defined(USE_CPP11)     // Support c++11 ?
		box_.emplace_back(t);
#else
		box_.push_back(t);
#endif
		size_++;

		if (notify_first) {
			if (!cond_.notify()) { abort(); }
			if (!lock_.unlock()) { abort(); }
		} else {
			if (!lock_.unlock()) { abort(); }
			if (!cond_.notify()) { abort(); }
		}

		return true;
	}

	/**
	 * Receive message object
	 * @param t {T&} Store result object when function returns true
	 * @param ms {int} When >= 0, set wait timeout (milliseconds),
	 *  otherwise wait forever until message object is read or error occurs
	 * @return {bool} Whether message object was obtained
	 * @override
	 */
	bool pop(T& t, int ms = -1) {
		long long us = ((long long) ms) * 1000;
		if (!lock_.lock()) { abort(); }
		while (true) {
			if (peek(t)) {
				if (!lock_.unlock()) { abort(); }
				return true;
			}

			// Note the call order, must call wait first then check wait_ms
			if (!cond_.wait(us, true) && us >= 0) {
				if (!lock_.unlock()) { abort(); }
				return false;
			}
		}
	}

	// @override
	size_t pop(std::vector<T>& out, size_t max, int ms) {
		long long us = ((long long) ms) * 1000;
		size_t n = 0;
		if (!lock_.lock()) { abort(); }
		while (true) {
			T t;
			if (peek(t)) {
				out.push_back(t);
				n++;
				if (max > 0 && n >= max) {
					if (!lock_.unlock()) { abort(); }
				}
				continue;
			}
			if (n > 0) {
				if (!lock_.unlock()) { abort(); }
				return n;
			}
			if (!cond_.wait(us, true) && us >= 0) {
				if (!lock_.unlock()) { abort(); }
				return n;
			}
		}
	}

	/**
	 * Return the number of messages currently in the message queue
	 * @return {size_t}
	 * @override
	 */
	size_t size() const {
		return size_;
	}

	// @override
	bool has_null() const {
		return true;
	}

public:
	void lock() {
		if (!lock_.lock()) { abort(); }
	}

	void unlock() {
		if (!lock_.unlock()) { abort(); }
	}

private:
	tbox2(const tbox2&) {}
	const tbox2& operator=(const tbox2&);
private:
	std::list<T> box_;
	size_t       size_;
	thread_mutex lock_;
	thread_cond  cond_;

	bool peek(T& t) {
		typename std::list<T>::iterator it = box_.begin();
		if (it == box_.end()) {
			return false;
		}
#if __cplusplus >= 201103L || defined(USE_CPP11)     // Support c++11 ?
		t = std::move(*it);
#else
		t = *it;
#endif
		box_.erase(it);
		size_--;
		return true;
	}
};

} // namespace acl

