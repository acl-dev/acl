#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <list>

namespace acl {

class event_task;

class ACL_CPP_API event_timer : public noncopyable {
public:
	/**
	 * Constructor
	 * @param keep {bool} Whether this timer allows automatic restart
	 */
	event_timer(bool keep = false);
	virtual ~event_timer();

	/**
	 * Callback function when number of tasks in timer is empty.
	 * Subclasses can release in it. Once this function is called,
	 * it means this timer and all timer tasks in it are deleted from
	 * timer collection
	 * This function is triggered under three conditions:
	 * 1) When number of all tasks in timer is 0
	 * 2) When repeat timer is not set and a timer task in this timer is triggered
	 * 3) When del_timer(event_timer*) is called
	 */
	virtual void destroy(void) {}

	/**
	 * Whether tasks in timer are empty
	 * @return {bool}
	 */
	bool empty(void) const {
		return tasks_.empty();
	}

	/**
	 * Number of tasks in timer
	 * @return {size_t}
	 */
	size_t length(void) const {
		return length_;
	}

	/**
	 * Whether this timer is auto-restart
	 * @param on {bool}
	 */
	void keep_timer(bool on);

	/**
	 * Determine whether this timer is auto-restart
	 * @return {bool}
	 */
	bool keep_timer(void) const {
		return keep_;
	}

	/**
	 * Clear timer tasks in timer
	 * @return {int} Number of cleared timer tasks
	 */
	int clear(void);

	/**
	 * Subclasses must implement this callback function. Note: Subclasses or
	 * callers are prohibited from calling
	 * event_timer's destructor function inside timer_callback,
	 * otherwise it will cause major problems
	 * @param id {unsigned int} ID number corresponding to a task
	 */
	virtual void timer_callback(unsigned int id) = 0;

	/****************************************************************/
	/*        Subclasses can call the following functions to add some new timer task ID numbers      */
	/****************************************************************/
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Add new task ID number for this timer, so that multiple timer tasks can be
	 * started through one timer
	 * @param id {unsigned int} Timer task ID number
	 * @param delay {__int64} How often to automatically trigger this timer, and
	 * pass back the corresponding timer task
	 *  ID number (microseconds)
	 * @return {__int64} How long until the first timer task ID of this timer will
	 * be triggered (microseconds)
	 */
	__int64 set_task(unsigned int id, __int64 delay);

	/**
	 * Delete timer task corresponding to a message ID in timer
	 * @param {unsigned int} Timer task ID
	 * @return {__int64} How long until the first timer task ID of this timer will
	 * be triggered (microseconds)
	 */
	__int64 del_task(unsigned int id);
#else
	long long int set_task(unsigned int id, long long int delay);
	long long int del_task(unsigned int id);
#endif

	//////////////////////////////////////////////////////////////////////

	/**
	 * Trigger all expired timer tasks
	 * @return {long lont int} Next task that will expire
	 */
#if defined(_WIN32) || defined(_WIN64)
	__int64 trigger(void);
	__int64 min_delay(void) const {
		return min_delay_;
	}
#else
	long long int trigger(void);
	long long int min_delay(void) const {
		return min_delay_;
	}
#endif

protected:
#if defined(_WIN32) || defined(_WIN64)
	__int64 min_delay_;
	__int64 present_;
#else
	long long int min_delay_;
	long long int present_;
#endif

private:
	size_t length_;
	std::list<event_task*> tasks_;
	bool keep_;  // Whether this timer allows automatic restart
#if defined(_WIN32) || defined(_WIN64)
	__int64 set_task(event_task* task);
#else
	long long int set_task(event_task* task);
#endif

	/**
	 * Set current timer timestamp
	 */
	void set_time(void);
};

}  // namespace acl

