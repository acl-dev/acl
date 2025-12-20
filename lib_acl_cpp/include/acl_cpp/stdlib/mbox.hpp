#pragma once
#include "../acl_cpp_define.hpp"
#include <cassert>
#include "box.hpp"

namespace acl {

// internal functions being used
void*  mbox_create(bool mpsc);
void   mbox_free(void*, void (*free_fn)(void*));
bool   mbox_send(void*, void*);
void*  mbox_read(void*, int, bool*);
size_t mbox_nsend(void*);
size_t mbox_nread(void*);

/**
 * Class that can be used for communication between threads and between
 * coroutines. Internal implementation uses lock-free queue + IO communication
 * combined approach
 *
 * Example:
 *
 * class myobj {
 * public:
 *     myobj() {}
 *     ~myobj() {}
 *     
 *     void run()
 *     {
 *         printf("hello world!\r\n");
 *     }
 * };
 * 
 * acl::mbox<myobj> mbox;
 *
 * void thread_producer()
 * {
 *     myobj* o = new myobj;
 *     mbox.push(o);
 * }
 * 
 * void thread_consumer()
 * {
 *     myobj* o = mbox.pop();
 *     o->run();
 *     delete o;
 * }
 */

template<typename T>
class mbox : public box<T> {
public:
	/**
	 * Constructor
	 * @param free_obj {bool} When tbox is destroyed, whether to automatically
	 * check and release
	 *  unconsumed dynamic objects
	 * @param mpsc {bool} Whether it is multi-producer-single-consumer mode
	 */
	explicit mbox(bool free_obj = true, bool mpsc = true)
	: free_obj_(free_obj)
	{
		mbox_ = mbox_create(mpsc);
		assert(mbox_);
	}

	~mbox() {
		mbox_free(mbox_, free_obj_ ? mbox_free_fn : NULL);
	}

	/**
	 * Send message object
	 * @param t {T*} Non-empty message object
	 * @return {bool} Whether send was successful
	 * @override
	 */
	bool push(T* t, bool dummy = false) {
        (void) dummy;
		return mbox_send(mbox_, t);
	}

	/**
	 * Receive message object
	 * @param ms {int} When >= 0, set read wait timeout (milliseconds), otherwise
	 *  wait forever until message object is read or error occurs
	 * @param success {bool*} Can be used to help determine whether read operation
	 * was successful
	 * @return {T*} Non-NULL indicates a message object was read. When NULL, need
	 * to check through
	 *  success parameter return value whether operation was successful
	 * @override
	 */
	T* pop(int ms = -1, bool* success = NULL) {
		return (T*) mbox_read(mbox_, ms, success);
	}

	// @override
	size_t pop(std::vector<T*>& out, size_t max, int ms) {
		size_t n = 0;
		bool success;
		while (true) {
			T* t = (T*) mbox_read(mbox_, ms, &success);
			if (! t) {
				return n;
			}

			out.push_back(t);
			n++;
			if (max > 0 && n >= max) {
				return n;
			}
			ms = 0;
		}
	}

	/**
	 * mbox does not support passing null messages
	 * @return {bool}
	 * @override
	 */
	bool has_null() const {
		return false;
	}

	/**
	 * Count the number of messages currently sent
	 * @return {size_t}
	 */
	size_t push_count() const {
		return mbox_nsend(mbox_);
	}

	/**
	 * Count the number of messages currently received
	 * @return {size_t}
	 */
	size_t pop_count() const {
		return mbox_nread(mbox_);
	}

private:
	void* mbox_;
	bool  free_obj_;

	static void mbox_free_fn(void* o) {
		T* t = (T*) o;
		delete t;
	}
};

} // namespace acl

