#pragma once
#include "../acl_cpp_define.hpp"
#include "noncopyable.hpp"

#if !defined(_WIN32) && !defined(_WIN64)
# include <pthread.h>
# ifndef	acl_pthread_mutex_t
#  define	acl_pthread_mutex_t	pthread_mutex_t
# endif
#else
struct acl_pthread_mutex_t;
#endif

namespace acl {

/**
 * Thread mutex.
 */
class ACL_CPP_API thread_mutex : public noncopyable {
public:
	/**
	 * Constructor.
	 * @param recursive {bool} Whether to use recursive lock mode.
	 */
	thread_mutex(bool recursive = true);
	~thread_mutex();

	/**
	 * Lock in current thread. Will wait until successful. Internal failure (generally does not fail, unless system error).
	 * @return {bool} Returns false to indicate thread lock failed.
	 */
	bool lock();

	/**
	 * Try to lock. Returns immediately regardless of success or failure.
	 * @return {bool} Returns true to indicate lock successful. Returns false to indicate lock failed.
	 */
	bool try_lock();

	/**
	 * Unlock thread lock.
	 * @return {bool} Returns false to indicate unlock failed, possibly because lock was not successful before.
	 */
	bool unlock();

	/**
	 * Get acl library's C version system type thread mutex.
	 * @return {acl_pthread_mutex_t*}
	 */
	acl_pthread_mutex_t* get_mutex() const;

private:
	acl_pthread_mutex_t* mutex_;
#if !defined(_WIN32) && !defined(_WIN64)
	pthread_mutexattr_t  mutex_attr_;
#endif
};

class ACL_CPP_API thread_mutex_guard : public noncopyable
{
public:
	thread_mutex_guard(thread_mutex& mutex);
	~thread_mutex_guard();

private:
	thread_mutex& mutex_;
};

} // namespace acl
