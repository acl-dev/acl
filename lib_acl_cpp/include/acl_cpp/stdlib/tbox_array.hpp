#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include <stdlib.h>
#include <string.h>
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
 * acl::tbox_array<myobj> tbox;
 *
 * void thread_producer() {
 *     myobj* o = new myobj;
 *     tbox.push(o);
 * }
 *
 * void thread_consumer() {
 *     myobj* o = tbox.pop();
 *     o->test();
 *     delete o;
 * }
 */

template<typename T>
class tbox_array : public box<T> {
public:
	/**
	 * Constructor
	 * @param free_obj {bool} When tbox_array is destroyed, whether to
	 * automatically check and release
	 *  unconsumed dynamic objects
	 */
	explicit tbox_array(bool free_obj = true)
	: capacity_(10000)
	, off_curr_(0)
	, off_next_(0)
	, waiters_(0)
	, free_obj_(free_obj)
	, cond_(&lock_)
	{
		array_ = (T**) malloc(sizeof(T*) * capacity_);
	}

	~tbox_array() {
		clear(free_obj_);
		free(array_);
	}

	/**
	 * Clear unconsumed message objects in the message queue
	 * @param free_obj {bool} Release message objects by calling delete method
	 */
	void clear(bool free_obj = false) {
		if (free_obj) {
			for (size_t i = off_curr_; i < off_next_; i++) {
				delete array_[i];
			}
		}
	}

	/**
	 * Send message object. This process unlocks first then notifies after adding
	 * object
	 * @param t {T*} Non-empty message object
	 * @param notify_first {bool} If true, notify first then unlock, otherwise
	 * unlock first
	 *  then notify. Note the difference between the two
	 * @return {bool}
	 * @override
	 */
	bool push(T* t, bool notify_first = false) {
		if (! lock_.lock()) { abort(); }

		if (off_next_ == capacity_) {
			if (off_curr_ >= 10000) {
#if 1
				size_t n = 0;
				for (size_t i = off_curr_; i < off_next_; i++) {
					array_[n++] = array_[i];
				}
#else
				memmove(array_, array_ + off_curr_,
					(off_next_ - off_curr_) * sizeof(T*));
#endif

				off_next_ -= off_curr_;
				off_curr_ = 0;
			} else {
				capacity_ += 10000;
				array_ = (T**) realloc(array_, sizeof(T*) * capacity_);
			}
		}
		array_[off_next_++] = t;

		if (notify_first) {
			if (! cond_.notify()) { abort(); }
			if (! lock_.unlock()) { abort(); }
		} else {
			if (! lock_.unlock()) { abort(); }
			if (! cond_.notify()) { abort(); }
		}

		return true;
	}

	/**
	 * Receive message object
	 * @param ms {int} When >= 0, set wait timeout (milliseconds),
	 *  otherwise wait forever until message object is read or error occurs
	 * @param found {bool*} When not NULL, used to store whether a message object
	 * was obtained, mainly used for
	 *  checking when passing null objects is allowed
	 * @return {T*} Non-NULL indicates a message object was obtained. When returns
	 * NULL, need to do further
	 * checking. If producer pushes a null object (NULL), consumer will also get
	 * NULL,
	 * but it is still considered that a message object was obtained, just that
	 * it's a null object. If wait_ms parameter
	 * is -1 and returns NULL, it is still considered that a null message object
	 * was obtained. If wait_ms is greater than
	 * or equal to 0 and returns NULL, then should check whether found parameter
	 * value is true or false to
	 *  determine whether a null message object was obtained
	 * @override
	 */
	T* pop(int ms = -1, bool* found = NULL) {
		long long us = ((long long) ms) * 1000;
		bool found_flag;

		if (! lock_.lock()) { abort(); }
		while (true) {
			T* t = peek(found_flag);
			if (found_flag) {
				if (! lock_.unlock()) { abort(); }
				if (found) {
					*found = found_flag;
				}
				return t;
			}

			// Note the call order, must call wait first then check wait_ms
			waiters_++;
			if (! cond_.wait(us, true) && us >= 0) {
				waiters_--;
				if (! lock_.unlock()) { abort(); }
				if (found) {
					*found = false;
				}
				return NULL;
			}
			waiters_--;
		}
	}

	// @override
	size_t pop( std::vector<T*>& out, size_t max, int ms) {
		long long us = ((long long) ms) * 1000;
		size_t n = 0;
		bool found_flag;

		if (! lock_.lock()) { abort(); }
		while (true) {
			T* t = peek(found_flag);
			if (found_flag) {
				out.push_back(t);
				n++;
				if (max > 0 && n >= max) {
					return n;
				}
				continue;
			}

			if (n > 0) {
				if (! lock_.unlock()) { abort(); }
				return n;
			}

			if (! cond_.wait(us, true) && us >= 0) {
				if (! lock_.unlock()) { abort(); }
				return n;
			}
		}
	}

	/**
	 * tbox supports passing null messages
	 * @return {bool}
	 * @override
	 */
	bool has_null() const {
		return true;
	}

	/**
	 * Return the number of messages currently in the message queue
	 * @return {size_t}
	 */
	size_t size() const {
		return off_next_ - off_curr_;
	}

public:
	void lock() {
		if (! lock_.lock()) { abort(); }
	}

	void unlock() {
		if (! lock_.unlock()) { abort(); }
	}

private:
	T**          array_;
	size_t       capacity_;
	size_t       off_curr_;
	size_t       off_next_;
	size_t       waiters_;
	bool         free_obj_;
	thread_mutex lock_;
	thread_cond  cond_;

	T* peek(bool& found_flag) {
		if (off_curr_ == off_next_) {
			found_flag = false;
			if (off_curr_ > 0) {
				off_curr_ = off_next_ = 0;
			}
			return NULL;
		}

		found_flag = true;
		T* t = array_[off_curr_++];
		return t;
	}
};

} // namespace acl

