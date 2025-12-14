#pragma once
#include "../acl_cpp_define.hpp"
#include "noncopyable.hpp"

#if !defined(_WIN32) && !defined(_WIN64)
# include <pthread.h>
# ifndef	acl_pthread_cond_t
#  define	acl_pthread_cond_t	pthread_cond_t
# endif
#else
struct acl_pthread_cond_t;
#endif

namespace acl {

class thread_mutex;

/**
 * Thread condition variable
 */
class ACL_CPP_API thread_cond : public noncopyable {
public:
	/**
	 * Constructor
	 * @param mutex {thread_mutex*} When this parameter is not NULL, internally automatically references
	 *  this thread lock, otherwise, internally creates thread lock
	 */
	thread_cond(thread_mutex* mutex = NULL);
	~thread_cond();

	/**
	 * Wait for thread condition variable to be ready
	 * @param microseconds {long long} Timeout for waiting condition variable to be ready (microseconds)
	 *   > 0 means wait timeout duration
	 *   == 0, don't wait
	 *   < 0 means wait forever until condition variable is ready
	 * @param locked {bool} This parameter indicates whether the lock has been locked. If not locked yet, then
	 *  internally will automatically lock first, then unlock before method returns; if externally already locked, then internally does not
	 *  perform lock/unlock operations on the mutex
	 * @return {bool} Returns true indicates condition variable is ready, otherwise indicates timeout or not notified
	 */
	bool wait(long long microseconds = -1, bool locked = false);

	/**
	 * Notify one or several threads waiting on the thread condition variable, indicating condition variable is ready
	 * @return {bool} Returns false indicates notification failed
	 */
	bool notify();

	/**
	 * Notify all threads waiting on the thread condition variable, indicating condition variable is ready
	 * @return {bool} Returns false indicates notification failed
	 */
	bool notify_all();

	/**
	 * Get the thread mutex bound to this thread condition variable
	 * @return {thread_mutex&}
	 */
	thread_mutex& get_mutex() const;

	/**
	 * Get system-type thread condition variable object
	 * @return {acl_pthread_cond_t*}
	 */
	acl_pthread_cond_t* get_cond() const;

private:
	thread_mutex* mutex_;
	thread_mutex* mutex_internal_;
	acl_pthread_cond_t* cond_;

	bool block_wait(bool locked);
	bool timed_wait(long long microseconds, bool locked);
};

} // namespace acl

