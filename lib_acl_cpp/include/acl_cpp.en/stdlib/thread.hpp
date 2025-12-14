#pragma once
#include "../acl_cpp_define.hpp"
#include "noncopyable.hpp"

namespace acl {

/**
 * Pure virtual function: Thread task class. The run method of instances of this
 * class is executed in child threads
 */
class ACL_CPP_API thread_job : public noncopyable {
public:
	thread_job() {}
	virtual ~thread_job() {}

	/**
	 * Pure virtual function, subclasses must implement this function. This
	 * function executes in child threads
	 * @return {void*} Parameter returned before thread exits
	 */
	virtual void* run() = 0;

	/**
	 * Virtual method, called before run() method is called in newly created child
	 * thread. In synchronous thread creation
	 * mode, after child thread is created, this virtual method is called, then the
	 * creating thread is notified,
	 * thus ensuring that child thread executes initialization process before
	 * creating thread's start() method returns.
	 */
	virtual void init() {}
};

template<typename T> class tbox;
class atomic_long;

/**
 * Thread pure virtual class. The interface definition of this class is similar
 * to Java's interface definition. Subclasses need to implement
 * pure virtual functions of the base class. Users start the thread process by
 * calling thread::start()
 */
class ACL_CPP_API thread : public thread_job {
public:
	thread();
	virtual ~thread();

	/**
	 * Start the thread process. Once this function is called, a new
	 * child thread will be started immediately, executing base class
	 * thread_job::run process in the child thread
	 * @return {bool} Whether thread was successfully created
	 */
	bool start(bool sync = false);

	/**
	 * When thread is created in non-detachable state, this function must be called
	 * to wait for thread to end;
	 * If thread is created in detachable state, calling this function is
	 * prohibited
	 * @param out {void**} When this parameter is not NULL pointer, this parameter
	 * is used to store
	 *  parameter returned before thread exits
	 * @return {bool} Whether successful
	 */
	bool wait(void** out = NULL);

	/**
	 * Call this function before calling start to set whether the created thread is
	 * detachable
	 * state. If this function is not called, the created thread defaults to
	 * non-detachable state. In non-detachable state,
	 * other threads can wait on this thread object, otherwise waiting on this
	 * thread object is prohibited. In non-
	 * detachable state, other threads must wait on this thread, otherwise it will
	 * cause memory leak.
	 * @param yes {bool} Whether it is detachable state
	 * @return {thread&}
	 */
	thread& set_detachable(bool yes);

	/**
	 * Call this function before calling start to set the stack size of the created
	 * thread
	 * @param size {size_t} Thread stack size. When this value is 0 or this
	 * function is not called, the created thread stack size is the system default
	 * value
	 * @return {thread&}
	 */
	thread& set_stacksize(size_t size);

	/**
	 * Call this function after calling start to get the id number of the created
	 * thread
	 * @return {unsigned long}
	 */
	unsigned long thread_id() const;

	/**
	 * Thread id number of the thread where the current caller is located
	 * @return {unsigned long}
	 */
	static unsigned long thread_self();
	static unsigned long self() {
		return thread_self();
	}

private:
	bool detachable_;
	size_t stack_size_;
#if defined(_WIN32) || defined(_WIN64)
	void* thread_;
	unsigned long thread_id_;
#else
	pthread_t thread_;
	unsigned long thread_id_;
#endif
	tbox<int>*   sync_;
	atomic_long* lock_;

	void* return_arg_;
	static void* thread_run(void* arg);

	void wait_for_running();
};

} // namespace acl

