#pragma once
#include "noncopyable.hpp"

struct acl_pthread_pool_t;
struct acl_pthread_pool_attr_t;

namespace acl {

class thread_job;

/**
 * Thread pool management class. Threads in the thread pool managed by this
 * class are semi-resident (i.e., threads automatically exit
 * after being idle for a certain time). This class has two non-pure virtual
 * functions: thread_on_init (called first when a thread in thread pool is
 * first created), thread_on_exit (called when a thread in thread pool exits)
 */
class ACL_CPP_API thread_pool : public noncopyable {
public:
	thread_pool();
	virtual ~thread_pool();

	/**
	 * Start thread pool. After creating thread pool object, must first call this
	 * function to start thread pool
	 */
	void start();

	/**
	 * Stop and destroy thread pool, and release thread pool resources. Calling
	 * this function can make all child threads exit,
	 * but does not release this instance. If this class instance is dynamically
	 * allocated, users should release the class instance themselves.
	 * After calling this function, if want to restart thread pool process, must
	 * call start process again
	 */
	void stop();

	/**
	 * Wait for all threads in thread pool to finish executing all tasks
	 */
	void wait();

	/**
	 * Submit a task to a thread in thread pool for execution. Threads in thread
	 * pool
	 * will execute the run function in this task
	 * @param job {thread_job*} Thread task
	 * @return {bool} Whether successful
	 */
	bool run(thread_job* job);

	/**
	 * Submit a task to a thread in thread pool for execution. Threads in thread
	 * pool
	 * will execute the run function in this task. This function has exactly the
	 * same functionality as run, only provided
	 * this interface to make JAVA programmers feel more familiar
	 * @param job {thread_job*} Thread task
	 * @return {bool} Whether successful
	 */
	bool execute(thread_job* job);

	/**
	 * Call this function before calling start to set the stack size of created
	 * threads
	 * @param size {size_t} Thread stack size. When this value is 0 or this
	 * function is not called, the created thread stack size is the system default
	 * value
	 * @return {thread&}
	 */
	thread_pool& set_stacksize(size_t size);

	/**
	 * Set maximum thread count limit for thread pool
	 * @param max {size_t} Maximum thread count. If this function is not called,
	 * internal default value is 100
	 * @return {thread_pool&}
	 */
	thread_pool& set_limit(size_t max);

	/**
	 * Get current thread pool maximum thread count limit
	 * @return {size_t}
	 */
	size_t get_limit() const;

	/**
	 * Set timeout exit time for idle threads in thread pool
	 * @param ttl {int} Idle timeout time (seconds). If this function is not
	 * called, internal default is 0
	 * @return {thread_pool&}
	 */
	thread_pool& set_idle(int ttl);

	/**
	 * Get number of child threads in current thread pool
	 * @return {int} Returns number of child threads in thread pool. If thread pool
	 * process has not been started through calling start,
	 *  this function returns -1
	 */
	int threads_count() const;

	/**
	 * Get number of unprocessed tasks in current thread pool
	 * @return {int} Returns -1 when thread pool has not been started (i.e., start
	 * not called) or has been destroyed
	 */
	int task_qlen() const;

protected:
	/**
	 * When a child thread in thread pool is first created, this virtual function
	 * will be called.
	 * Users can perform some initialization work in their implementation
	 * @return {bool} Whether initialization was successful
	 */
	virtual bool thread_on_init() { return true; }

	/**
	 * When a child thread in thread pool exits, this virtual function will be
	 * called. Users can
	 * perform some resource release work in their implementation
	 */
	virtual void thread_on_exit() {}

private:
	size_t stack_size_;
	size_t threads_limit_;
	int    thread_idle_;

	acl_pthread_pool_t* thr_pool_;
	acl_pthread_pool_attr_t* thr_attr_;

	static void thread_run(void* arg);
	static int  thread_init(void* arg);
	static void thread_exit(void* arg);
};

} // namespace acl

