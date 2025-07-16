#pragma once
#include <vector>
#include "fiber_cpp_define.hpp"
#include "fiber_mutex_stat.hpp"

struct ACL_FIBER_MUTEX;

namespace acl {

/**
 * The mutex lock can be used for mutual exclusion between coroutines within
 * the same thread and between coroutines of different threads, as well as
 * between threads and between coroutines and independent threads.
 */
class FIBER_CPP_API fiber_mutex {
public:
	/**
	 * The constructor.
	 * @param mutex {ACL_FIBER_MUTEX*} When not empty, C++ lock object will
	 *  be created using C lock object, otherwise C lock object will be
	 *  automatically created internally; If it is not empty, the C lock
	 *  object that should be passed in during the decomposition of this
	 *  object needs to be released by the application layer itself.
	 */
	explicit fiber_mutex(ACL_FIBER_MUTEX* mutex = NULL);
	~fiber_mutex();

	/**
	 * Lock the mutex.
	 * @return {bool} If lock successfully, return true, or return false.
	 */
	bool lock();

	/**
	 * Try to lock the mutex.
	 * @return {bool} If lock successfully, return true, or return false
	 *  if the mutex is locked by the other coroutine.
	 */
	bool trylock();

	/**
	 * Unlock the mutex and wake up the waiter.
	 * @return {bool} If unlock successfully, return true, or return false.
	 */
	bool unlock();

public:
	/**
	 * Return the C object of mutex.
	 * @return {ACL_FIBER_MUTEX*}
	 */
	ACL_FIBER_MUTEX* get_mutex() const {
		return mutex_;
	}

	/**
	 * Detect the deadlock state.
	 * @param out {fiber_mutex_stats&} Save the checking result.
	 * @return {bool} Returning true indicates the existence of a
	 *  deadlock issue.
	 */
	static bool deadlock(fiber_mutex_stats& out);

	/**
	 * Detect deadlocks and print all coroutine stacks that have entered
	 * a deadlock state to standard output.
	 */
	static void deadlock_show();

private:
	ACL_FIBER_MUTEX* mutex_;
	ACL_FIBER_MUTEX* mutex_internal_;

	fiber_mutex(const fiber_mutex&);
	void operator=(const fiber_mutex&);
};

class FIBER_CPP_API fiber_mutex_guard {
public:
	explicit fiber_mutex_guard(fiber_mutex& mutex) : mutex_(mutex) {
		mutex_.lock();
	}

	~fiber_mutex_guard() {
		mutex_.unlock();
	}

private:
	fiber_mutex& mutex_;
};

} // namespace acl
