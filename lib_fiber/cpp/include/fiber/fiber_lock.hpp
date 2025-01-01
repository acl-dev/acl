#pragma once
#include "fiber_cpp_define.hpp"

struct ACL_FIBER_LOCK;
struct ACL_FIBER_RWLOCK;

namespace acl {

/**
 * mutex lock that can only be used for mutual exclusion between coroutines
 * within the same thread.
 */
class FIBER_CPP_API fiber_lock {
public:
	fiber_lock();
	~fiber_lock();

	/**
	 * Lock the mutex.
	 * @return {bool} If lock successfully, return true, or return false.
	 */
	bool lock();

	/**
	 * Try to lock the mutex.
	 * @return {bool} If lock successfully, return true, or return false
	 *  if the mutex is locked by other coroutine.
	 */
	bool trylock();

	/**
	 * Unlock the mutex and wakeup the waiter.
	 * @return {bool} If unlock successfully, return true, or return false.
	 */
	bool unlock();

private:
	ACL_FIBER_LOCK* lock_;

	fiber_lock(const fiber_lock&);
	void operator=(const fiber_lock&);
};

/**
 * Read/write lock that can only be used for mutual exclusion between
 * coroutines within the same thread.
 */
class FIBER_CPP_API fiber_rwlock {
public:
	fiber_rwlock();
	~fiber_rwlock();

	/**
	 * Lock in read mode.
	 */
	void rlock();

	/**
	 * Try to lock in read mode.
	 * @return {bool} If lock successfully, return true, or return false
	 *  if the mutex is locked by other coroutine.
	 */
	bool tryrlock();

	/**
	 * Unlock read mode.
	 */
	void runlock();

	/**
	 * Lock in write mode.
	 */
	void wlock();

	/**
	 * Try to lock in write mode.
	 * @return {bool} If lock successfully, return true, or return false
	 *  if the mutex is locked by other coroutine.
	 */
	bool trywlock();

	/**
	 * Unlock write mode.
	 */
	void wunlock();

private:
	ACL_FIBER_RWLOCK* rwlk_;

	fiber_rwlock(const fiber_rwlock&);
	void operator=(const fiber_rwlock&);
};

} // namespace acl
