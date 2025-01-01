#pragma once
#include "fiber_cpp_define.hpp"

struct ACL_FIBER_COND;

namespace acl {

class fiber_mutex;

/**
 * Conditional variables that can be used between coroutines, threads, and
 * between coroutines and threads.
 */
class FIBER_CPP_API fiber_cond {
public:
	fiber_cond();
	~fiber_cond();

	/**
	 * Wait for the conditional variable be available.
	 * @param mutex {fiber_mutex&}
	 * @param timeout {int} The waiting timeout in milliseconds.
	 * @return {bool} Return true if avaialbe or return false if timeout.
	 */
	bool wait(fiber_mutex& mutex, int timeout = -1);

	/**
	 * Wake up the waiter on the condition variable, and if there are no
	 * waiters, return directly. The running behavior is similar to that
	 * of thread condition variables.
	 * @return {bool} Return true if successful or return false if error.
	 */
	bool notify();

public:
	/**
	 * Return the C object conditional variable.
	 * @return {ACL_FIBER_COND*}
	 */
	ACL_FIBER_COND* get_cond() const {
		return cond_;
	}

private:
	ACL_FIBER_COND* cond_;

	fiber_cond(const fiber_cond&);
	void operator=(const fiber_cond&);
};

}
