#pragma once
#include <stddef.h>
#include <string>
#include <vector>
#include "fiber_cpp_define.hpp"

struct ACL_FIBER;

namespace acl {

typedef enum {
	FIBER_EVENT_T_KERNEL,	// Linux: epoll, FreeBSD: kquque, Windows: iocp
	FIBER_EVENT_T_POLL,	// Linux, FreeBSD, MacOS, Windows
	FIBER_EVENT_T_SELECT,	// Linux, FreeBSD, MacOS, Windows
	FIBER_EVENT_T_WMSG,	// Windows
	FIBER_EVENT_T_IO_URING,	// Linux
} fiber_event_t;

struct FIBER_CPP_API fiber_frame {
	fiber_frame() : pc(0), off(0) {}
	~fiber_frame() {}

	std::string func;
	long pc;
	long off;
};

/**
 * Definition of coroutine class, pure virtual class, requires subclass
 * inheritance and implementation of pure virtual methods.
 */
class FIBER_CPP_API fiber {
public:
	fiber();
	explicit fiber(ACL_FIBER *fb);

	/**
	 * The constructor.
	 * @param running {bool} When it is true, it means that the current
	 *  coroutine has been started, and only a coroutine object is
	 *  declared for binding with the ACL_FIBER object. At this time,
	 *  calling the start method of this object to start a new coroutine
	 *  is prohibited; When false, the start method needs to be called to
	 *  start a new coroutine.
	 */
	explicit fiber(bool running);

	virtual ~fiber();

	/**
	 * When creating a coroutine class object with the construction
	 * parameter 'running' set to false, this function needs to be started.
	 * The run method of the subclass's overload will be called back, and
	 * if running is true, calling the start method will be prohibited.
	 * @param stack_size {size_t} The inital stack size of the coroutine.
	 * @param share_stack {bool} Whether to use shared stack mode to start
	 *  the current coroutine.
	 */
	void start(size_t stack_size = 320000, bool share_stack = false);

	/**
	 * Calling the method to stop the currenct running coroutine.
	 * @param sync {bool} Whether to use synchronous mode, that is, whether
	 *  to wait for the killed coroutine to resturn.
	 * @return {bool} Return false to indicate that the coroutine has not
	 *  been started or has already exited.
	 */
	bool kill(bool sync = false);

	/**
	 * Check if the current coroutine has been notified to exit.
	 * @return {bool} If the current coriutine has been notified to exit.
	 */
	bool killed() const;

	/**
	 * Check if the currently running coroutine has been notified to exit.
	 * @return {bool}
	 */
	static bool self_killed();

	/**
	 * Get the current coroutine's ID number.
	 * @return {unsigned int}
	 */
	unsigned int get_id() const;

	/**
	 * Get the current running coroutine's ID number.
	 * @return {unsigned int}
	 */
	static unsigned int self();

	/**
	 * Get the specified coroutine's ID number.
	 * @return {unsigned int}
	 */
	static unsigned int fiber_id(const fiber& fb);

	/**
	 * Get the error number of the current coroutine after executing a
	 * system API error.
	 * return {int}
	 */
	int get_errno() const;

	/**
	 * Check if the coroutine is in the waiting queue for scheduing.
	 * @return {bool}
	 */
	bool is_ready() const;

	/**
	 * Check if the coroutine if in the suspending status.
	 * @return {bool}
	 */
	bool is_suspended() const;

	/**
	 * Set the error number of the coroutine.
	 * @param errnum {int}
	 */
	void set_errno(int errnum);

	/**
	 * Clean up the error flag in the coroutine.
	 */
	static void clear();

public:
	/**
	 * Get the error message of the coroutine after calling system API.
	 * @return {const char*}
	 */
	static const char* last_serror();

	/**
	 * Get the error number of the coroutine after calling system API.
	 * @return {int}
	 */
	static int last_error();

	/**
	 * Convert the given error number into descriptive information.
	 * @param errnum {int} The given error number.
	 * @param buf {char*} Will save the result.
	 * @param size {size_t} The buf's space size.
	 * @return {const char*} Return the buf's address.
	 */
	static const char* strerror(int errnum, char* buf, size_t size);

	/**
	 * Output error messages to standard output.
	 * @param on {bool} If true, the internal error messages will be output
	 *  to the standard output.
	 */
	static void stdout_open(bool on);

	/**
	 * Set the maximum number of file handles that can be created by
	 * this process.
	 * @param max {int} Only valid when max >= 0.
	 * @return {int} Return the maximum number of current available handles.
	 */
	static int set_fdlimit(int max);

	/**
	 * Explicitly set the type of the coroutine scheduing event engine and
	 * set the coroutine scheduler to self start mode, which means that
	 * there is no need to explicitly call scheule() or schedule_with()
	 * to start the coroutine scheduing after creating one coroutine.
	 * @param type {fiber_event_t} See FIBER_EVENT_T_XXX above.
	 * @param schedule_auto {bool} If true, after creating a coroutine
	 *  object and running it, there is no need to explicitly call
	 *  scheduler/schedule-with to start all coroutine processes. The
	 *  coroutine scheduler will automatically start internally;
	 *  Otherwise, after creating and starting a coroutine, it is
	 *  necessary to explicitly call the schedule or schedule_with method
	 *  to start the coroutine scheduler to run the coroutine process;
	 *  The internal default state is false.
	 */
	static void init(fiber_event_t type, bool schedule_auto = false);

	/**
	 * On the Windows platform, this method can be called to enable GUI
	 * coroutine mode.
	 */
	static void schedule_gui();

	/**
	 * Start coroutine scheduing process.
	 * @param type {fiber_event_t} The event engine, see FIBER_EVENT_T_XXX.
	 */
	static void schedule(fiber_event_t type = FIBER_EVENT_T_KERNEL);

	/**
	 * Start coroutine scheduing process with the specified event type.
	 * @param type {fiber_event_t} The event engine, see FIBER_EVENT_T_XXX.
	 */
	static void schedule_with(fiber_event_t type);

	/**
	 * Check whether the current thread is in a coroutine scheduing state.
	 * @return {bool}
	 */
	static bool scheduled();

	/**
	 * Stop coroutine scheduing process.
	 */
	static void schedule_stop();

public:
	/**
	 * Give up the scheudling right of the current running coroutine.
	 */
	static void yield();

	/**
	 * Suspend the current coroutine and execute the next coroutine
	 * in the waiting queue
	 */
	static void switch_to_next();

	/**
	 * Append the specified coroutine to pending queue for execution.
	 * @param f {fiber&}
	 */
	static void ready(fiber& f);

	/**
	 * Let the current running coroutine sleep for a while in milliseconds.
	 * @param milliseconds {size_t} Specify the number of milliseconds to sleep.
	 * @return {size_t} The number of milliseconds remaining after waking
	 *  up again after the coroutine goes into sleep.
	 */
	static size_t delay(size_t milliseconds);

	/**
	 * Get the number of active coroutines.
	 * @return {unsigned}
	 */
	static unsigned alive_number();

	/**
	 * Get the number of coroutines in exiting state.
	 * @return {unsigned}
	 */
	static unsigned dead_number();

	/**
	 * Set all coroutines in this thread to use a non blocking method
	 * with timeout when connecting to the server.
	 * @param yes {bool}
	 * Notice: The method can only be used on Windows platform.
	 */
	static void set_non_blocking(bool yes);

	/**
	 * Set the size of the shared stack in shared stack mode, with an
	 * internal default value of 1024000 bytes.
	 * @param size {size_t} The shared stack's size.
	 */
	static void set_shared_stack_size(size_t size);

	/**
	 * Get the size of the shared stack in shared stack mode.
	 * @return {size_t} If 0 is returned, it means that the shared stack
	 *  mode is not enabled.
	 */
	static size_t get_shared_stack_size();

	static void acl_io_hook();

	static void acl_io_unlock();

	/**
	 * On the Windows platform, this function can be explicitly called to
	 * hook some system APIs related to network coroutines.
	 * @return {bool}
	 */
	static bool winapi_hook();

	/**
	 * Get the system error number.
	 * @return {int}
	 */
	static int  get_sys_errno();

	/**
	 * Set the system error number.
	 * @param errnum {int}
	 */
	static void set_sys_errno(int errnum);

	/**
	 * If multiple fibers can share one epoll handle in the same thread.
	 * @param yes {bool}
	 */
	static void share_epoll(bool yes);

public:
	/**
	 * Return the corresponding C language coroutine object of
	 * this coroutine object.
	 * @return {ACL_FIBER* }
	 */
	ACL_FIBER* get_fiber() const;

	/**
	 * Call C API to crete one coroutine.
	 * @param fn {void (*)(ACL_FIBER*, void*)} Execution entry of coroutine.
	 * @param ctx {void*} The context for execution function.
	 * @param size {size_t} The stack size of coroutine.
	 * @param share_stack {bool} If to create a coroutine in shared stack.
	 * @return {ACL_FIBER*}
	 */
	static ACL_FIBER* fiber_create(void (*fn)(ACL_FIBER*, void*),
			void* ctx, size_t size, bool share_stack = false);

	/**
	 * Get the coroutine's stacks.
	 * @param fb {const fiber&}
	 * @param out {std::vector<stack_frame>&} Save the result.
	 * @param max {size_t} Set the maximum deepth of the stacks.
	 */
	static void stacktrace(const fiber& fb, std::vector<fiber_frame>& out,
			size_t max = 50);

	/**
	 * Output the coroutine's calling stacks to stantard out.
	 * @param fb {const fiber&}
	 * @param max {size_t} The maximum deepth of the stacks to be displayed.
	 */
	static void stackshow(const fiber& fb, size_t max = 50);

protected:
	/**
	 * Virtual function, subclasses must implement this function. When the
	 * coroutine is started by calling the start() method, this virtual
	 * function will be called to notify the subclass that the coroutine
	 * has been started; If the parameter running in the constructor is
	 * true, start will be prohibited from being called, so this virtual
	 * method will not be called either.
	 */
	virtual void run();

private:
	ACL_FIBER* f_;

	fiber(const fiber&);
	void operator = (const fiber&);

	static void fiber_callback(ACL_FIBER* f, void* ctx);
};

/**
 * A coroutine class that can be used as a timer.
 */
class FIBER_CPP_API fiber_timer {
public:
	fiber_timer();
	virtual ~fiber_timer() {}

	/**
	 * Start one coroutine timer.
	 * @param milliseconds {unsigned int} in milliseconds.
	 * @param stack_size {size_t} The coroutine's stack size.
	 */
	void start(unsigned int milliseconds, size_t stack_size = 320000);

protected:
	/**
	 * The subclass must implement this pure virtual method, which will
	 * be called back when the timer starts.
	 */
	virtual void run() = 0;

private:
	ACL_FIBER* f_;

	fiber_timer(const fiber_timer&);
	void operator = (const fiber_timer&);

	static void timer_callback(ACL_FIBER* f, void* ctx);
};

#if defined(ACL_CPP_API)

/**
 * Timer management coroutine class.
 */
template <typename T>
class fiber_trigger : public fiber {
public:
	fiber_trigger(timer_trigger<T>& timer)
	: delay_(100)
	, stop_(false)
	, timer_(timer)
	{
	}

	virtual ~fiber_trigger() {}

	void add(T* o) {
		mbox_.push(o);
	}

	void del(T* o) {
		timer_.del(o);
	}

	timer_trigger<T>& get_trigger() {
		return timer_;
	}

	// @override
	void run() {
		while (!stop_) {
			T* o = mbox_.pop(delay_);
			if (o) {
				timer_.add(o);
			}

			long long next = timer_.trigger();
			long long curr = get_curr_stamp();
			if (next == -1) {
				delay_ = 100;
			} else {
				delay_ = next - curr;
				if (delay_ <= 0) {
					delay_ = 1;
				}
			}
		}
	}

private:
	long long delay_;
	bool stop_;

	timer_trigger<T>& timer_;
	mbox<T> mbox_;
};

#endif // ACL_CPP_API

} // namespace acl
