#pragma once
#include "fiber_cpp_define.hpp"

struct ACL_FIBER_EVENT;

namespace acl {

/**
 * An event-mixed lock that can be used between coroutines, between threads,
 * and between coroutines and threads to synchronize through event
 * waiting/notification.
 */
class FIBER_CPP_API fiber_event {
public:
	/**
	 * The constructor.
	 * @param use_mutex {bool} When used for event synchronization between
	 *  multiple threads, if the number of threads started is large 
	 *  (hundreds or thousands of threads), this flag should be set to true
	 *  so that the internal mutex lock is used for protection when
	 *  synchronizing internal objects to avoid the herd phenomenon. If the
	 *  number of threads started is large but the flag is false, the internal
	 *  atomic number is used for synchronization protection, which can easily
	 *  cause the herd problem. When the number of threads started is large
	 *  (about dozens of threads), this parameter can be set to false to tell
	 *  the internal atomic number to be used for synchronization protection.
	 * @param fatal_on_error {bool} Whether to crash directly when an internal
	 *  error occurs to facilitate error debugging by developers.
	 */
	fiber_event(bool use_mutex = true, bool fatal_on_error = true);
	~fiber_event();

	/**
	 * Waiting for event lock.
	 * @return {bool} Return true if locking successfully, otherwise it
	 *  indicates an internal error.
	 */
	bool wait();

	/**
	 * Try waiting for the event lock.
	 * @return {bool} Return true if locking successfully, otherwise it means
	 *  the lock is currently in use.
	 */
	bool trywait();

	/**
	 * The event lock owner releases the event lock and notifies the waiter.
	 * @return {bool} Return true if the notification is successful, otherwise
	 *  it indicates an internal error.
	 */
	bool notify();

public:
	/**
	 * Returns the the event object of C version.
	 * @return {ACL_FIBER_EVENT*}
	 */
	ACL_FIBER_EVENT* get_event() const {
		return event_;
	}

private:
	ACL_FIBER_EVENT* event_;

	fiber_event(const fiber_event&);
	void operator=(const fiber_event&);
};

} // namespace acl

