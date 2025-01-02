#pragma once
#include "fiber_cpp_define.hpp"
#include <list>
#include <cstdlib>
#include "fiber.hpp"
#include "fiber_mutex.hpp"
#include "fiber_cond.hpp"

namespace acl {

/**
 * Used for message cmmunication between coroutines, threads, and between
 * coroutines, implemented through coroutine condition variables and
 * coroutine lock.
 *
 * Sample:
 *
 * class myobj {
 * public:
 *     myobj() {}
 *     ~myobj() {}
 *
 *     void test() { printf("hello world\r\n"); }
 * };
 *
 * acl::fiber_tbox<myobj> fiber_tbox;
 *
 * void thread_producer() {
 *     myobj* o = new myobj;
 *     fiber_tbox.push(o);
 * }
 *
 * void thread_consumer() {
 *     myobj* o = fiber_tbox.pop();
 *     o->test();
 *     delete o;
 * }
 */

// The base box<T> defined in acl_cpp/stdlib/box.hpp, so you must include
// box.hpp first before including fiber_tbox.hpp
template<typename T>
class fiber_tbox : public box<T> {
public:
	/**
	 * The constructor
	 * @param free_obj {bool} Automatically check and release consumed
	 *  dynamic objects when fiber_tbox is destroyed.
	 */
	explicit fiber_tbox(bool free_obj = true) : size_(0), free_obj_(free_obj) {}

	~fiber_tbox() {
		clear(free_obj_);
	}

	/**
	 * Clean up the unconsumed message objects in the message queue.
	 * @param free_obj {bool} if deleting objects in queue when clean up.
	 */
	void clear(bool free_obj = false) {
		if (free_obj) {
			for (typename std::list<T*>::iterator it =
				tbox_.begin(); it != tbox_.end(); ++it) {

				delete *it;
			}
		}
		tbox_.clear();
	}

	/**
	 * Send message object
	 * @param t {T*} The object to be sent to queue.
	 * @param notify_first {bool} If this parameter is true, the internal
	 *  message will be notified first and then unlocked. Otherwise,
	 *  the unlocking first and then notification method will be used.
	 *  When the lifespan of the fiber_tbox object is relatively long,
	 *  setting this parameter to false is more efficient. If the lifespan
	 *  of the fiber_tbox object is short (such as directly destroying
	 *  the fiber_tbox object after waiting for the caller to call pop),
	 *  this parameter should be set to true to avoid unauthorized memory
	 *  access caused by the premature destruction of the fiber_tbox
	 *  object before the pusher has fully returned.
	 * @return {bool}
	 * @override
	 */
	bool push(T* t, bool notify_first = true) {
		if (! mutex_.lock()) { abort(); }

		tbox_.push_back(t);
		size_++;

		if (notify_first) {
			if (! cond_.notify()) { abort(); }
			if (! mutex_.unlock()) { abort(); }
		} else {
			if (! mutex_.unlock()) { abort(); }
			if (! cond_.notify()) { abort(); }
		}

		return true;
	}

	/**
	 * Receive message object from the message queue.
	 * @param ms {int} When ms >= 0,set the wait timout(in milliseconds),
	 *  otherwise wait forever until the message object is read or timeout
	 *  occcurs.
	 * @param found {bool*} The non-null space is used to store whether
	 *  a message object has been obtained, mainly for checking when null
	 *  objects are allowed to be passed
	 * @return {T*} Non NULL means obtaining a message object, and further
	 *  checks are required when returning NULL. If the producer pushes
	 *  an empty object (NULL), the consumer will also receive NULL, but
	 *  still consider that a message object has been obtained, which is
	 *  just an empty object; If the ms parameter is -1 returns NULL, it
	 *  is still considered that an empty message object has been obtained.
	 *  If ms is >= 0 and returns NULL, the value of the found parameter
	 *  should be checked to determine whether an empty message object
	 *  has been obtained.
	 * @override
	 */
	T* pop(int ms = -1, bool* found = NULL) {
		bool found_flag;

		if (! mutex_.lock()) { abort(); }

		while (true) {
			T* t = peek(found_flag);
			if (found_flag) {
				if (! mutex_.unlock()) { abort(); }
				if (found) {
					*found = found_flag;
				}
				return t;
			}

			// The calling order: wait should be called first
			// before checking wait_ms.
			if (! cond_.wait(mutex_, ms) && ms >= 0) {
				if (! mutex_.unlock()) { abort(); }
				if (found) {
					*found = false;
				}
				return NULL;
			}

			if (fiber::self_killed()) {
				if (! mutex_.unlock()) { abort(); }
				if (found) {
					*found = false;
				}
				return NULL;
			}
		}
	}

	// @override
	size_t pop(std::vector<T*>& out, size_t max, int ms) {
		size_t n = 0;
		bool found_flag;

		if (! mutex_.lock()) { abort(); }

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
				if (! mutex_.unlock()) { abort(); }
				return n;
			}

			if (! cond_.wait(mutex_, ms) && ms >= 0) {
				if (! mutex_.unlock()) { abort(); }
				return n;
			}

			if (fiber::self_killed()) {
				if (! mutex_.unlock()) { abort(); }
				return n;
			}
		}
	}

	/**
	 * tbox support transferring null message object.
	 * @return {bool}
	 * @override
	 */
	bool has_null() const {
		return true;
	}

	/**
	 * Return the current number of messages in the message queue.
	 * @return {size_t}
	 */
	size_t size() const {
		return size_;
	}

public:
	void lock() {
		if (! mutex_.lock()) {
			abort();
		}
	}

	void unlock() {
		if (! mutex_.unlock()) {
			abort();
		}
	}

private:
	fiber_tbox(const fiber_tbox&) {}
	const fiber_tbox& operator=(const fiber_tbox&);

private:
	std::list<T*> tbox_;
	size_t        size_;
	bool          free_obj_;
	fiber_mutex   mutex_;
	fiber_cond    cond_;

	T* peek(bool& found_flag) {
		typename std::list<T*>::iterator it = tbox_.begin();
		if (it == tbox_.end()) {
			found_flag = false;
			return NULL;
		}
		found_flag = true;
		size_--;
		T* t = *it;
		tbox_.erase(it);
		return t;
	}
};

} // namespace acl
