#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include "aio_delay_free.hpp"

namespace acl {

class aio_timer_task;
class aio_handle;

/**
 * Timer callback class
 */
class ACL_CPP_API aio_timer_callback : public aio_delay_free {
public:
	/**
	 * Constructor
	 * @param keep {bool} Whether this timer allows automatic restart
	 */
	explicit aio_timer_callback(bool keep = false);
	virtual ~aio_timer_callback();

	/**
	 * Callback function when number of tasks in timer is empty.
	 * Subclasses can release in it. Once this function is called,
	 * it means this timer and all timer tasks in it are deleted from
	 * timer collection
	 * This function is triggered under three conditions:
	 * 1) When number of all tasks in timer is 0 (e.g.,
	 *    del_timer(aio_timer_callback*, unsigned int) is
	 *    called and number of tasks is 0)
	 * 2) When aio_handle has not set repeat timer and a
	 *    timer task in this timer is triggered
	 * 3) When del_timer(aio_timer_callback*) is called
	 */
	virtual void destroy() {}

	/**
	 * Whether tasks in timer are empty
	 * @return {bool}
	 */
	bool empty() const;

	/**
	 * Number of tasks in timer
	 * @return {size_t}
	 */
	size_t length() const;

	/**
	 * Whether this timer is auto-restart
	 * @param on {bool}
	 */
	void keep_timer(bool on);

	/**
	 * Determine whether this timer is auto-restart
	 * @return {bool}
	 */
	bool keep_timer() const;

	/**
	 * Clear timer tasks in timer
	 * @return {int} Number of cleared timer tasks
	 */
	int clear();

protected:
	friend class aio_handle;

	/**
	 * Subclasses must implement this callback function. Note: Subclasses or callers are prohibited from calling
	 * aio_timer_callback's destructor function inside timer_callback,
	 * otherwise it will cause major problems
	 * @param id {unsigned int} ID number corresponding to a task
	 */
	virtual void timer_callback(unsigned int id) = 0;

	/****************************************************************/
	/*        Subclasses can call the following functions to add some new timer task ID numbers              */
	/****************************************************************/
#if defined(_WIN32) || defined(_WIN64)
	__int64 present_;

	/**
	 * Add new task ID number for this timer, so that multiple timer tasks can be started through one timer
	 * @param id {unsigned int} Timer task ID number
	 * @param delay {__int64} How often to automatically trigger this timer, and pass back the corresponding timer task
	 *  ID number (microseconds)
	 * @return {__int64} How long until the first timer task ID of this timer will be triggered (microseconds)
	 */
	__int64 set_task(unsigned int id, __int64 delay);

	/**
	 * Delete timer task corresponding to a message ID in timer
	 * @param {unsigned int} Timer task ID
	 * @return {__int64} How long until the first timer task ID of this timer will be triggered (microseconds)
	 */
	__int64 del_task(unsigned int id);
#else
	long long int present_;
	long long int set_task(unsigned int id, long long int delay);
	long long int del_task(unsigned int id);
#endif

	/**
	 * Set current timer timestamp
	 */
	void set_time();

private:
	aio_handle* handle_;
	size_t length_;
	std::list<aio_timer_task*> tasks_;
	bool keep_;  // Whether this timer allows automatic restart
	bool destroy_on_unlock_;  // Whether to destroy after unlock
#if defined(_WIN32) || defined(_WIN64)
	__int64 set_task(aio_timer_task* task);
	__int64 trigger(void);
#else
	long long int set_task(aio_timer_task* task);
	long long int trigger();
#endif
};

} // namespace acl

