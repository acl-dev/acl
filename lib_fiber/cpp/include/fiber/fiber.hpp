#pragma once
#include <stddef.h>
#include <string>
#include <vector>
#include "fiber_cpp_define.hpp"

struct ACL_FIBER;

namespace acl {

/**
 * fiber_event_t - Specifies which underlying event mechanism the fiber scheduler should use.
 * Different platforms support different event engines.
 */
typedef enum {
	FIBER_EVENT_T_KERNEL,   // Platform-specific: Linux: epoll, FreeBSD: kqueue, Windows: iocp (best performance)
	FIBER_EVENT_T_POLL,     // poll() - Available on: Linux, FreeBSD, MacOS, Windows (good compatibility)
	FIBER_EVENT_T_SELECT,   // select() - Available on: Linux, FreeBSD, MacOS, Windows (maximum compatibility)
	FIBER_EVENT_T_WMSG,     // Windows message loop - Windows only (for GUI applications)
	FIBER_EVENT_T_IO_URING, // io_uring - Linux 5.1+ only (highest performance on modern Linux)
} fiber_event_t;

/**
 * fiber_frame - Represents a single stack frame in a fiber's call stack
 * 
 * Used for stack tracing and debugging to understand the execution path
 * of a fiber.
 */
struct FIBER_CPP_API fiber_frame {
	fiber_frame() : pc(0), off(0) {}
	~fiber_frame() {}

	std::string func;  // Function name
	long pc;           // Program counter (instruction address)
	long off;          // Offset within the function
};

/**
 * fiber - Coroutine (fiber) class for lightweight concurrent programming
 * 
 * ============================================================================
 * OVERVIEW
 * ============================================================================
 * 
 * The fiber class provides a C++ interface for creating and managing coroutines
 * (also called fibers). Fibers are lightweight threads of execution that are
 * cooperatively scheduled within a single OS thread, providing:
 * 
 * - Lightweight concurrency: Thousands of fibers with minimal memory overhead
 * - Cooperative scheduling: Explicit yield points, no preemption
 * - Synchronous programming model: Write async code that looks synchronous
 * - Stack preservation: Each fiber has its own stack
 * 
 * Key Concepts:
 * - Fiber: A lightweight coroutine with its own stack
 * - Scheduler: Manages fiber execution within a thread
 * - Event loop: Handles I/O events and timer events
 * - Yield: Voluntarily give up CPU to allow other fibers to run
 * 
 * Usage Patterns:
 * 1. Inherit from fiber and override run() method
 * 2. Create fiber object and call start()
 * 3. Call fiber::schedule() to start the scheduler
 * 4. Or use go_fiber macros for simpler syntax
 * 
 * Thread Safety:
 * - Each thread has its own fiber scheduler
 * - Fibers within a thread share the same scheduler
 * - Use fiber_mutex for cross-thread synchronization
 * - Use fiber_lock for same-thread synchronization
 */
class FIBER_CPP_API fiber {
public:
	/**
	 * Default Constructor - Create a fiber that needs to be started
	 * 
	 * Creates a fiber object that is not yet running. You must call start()
	 * to begin execution.
	 */
	fiber();
	
	/**
	 * Constructor - Wrap an existing C fiber object
	 * 
	 * @param fb Pointer to an existing ACL_FIBER structure
	 */
	explicit fiber(ACL_FIBER *fb);

	/**
	 * Constructor - Create a fiber with specified running state
	 * 
	 * @param running {bool} Fiber state:
	 *  - true: Fiber is already running (wrapper mode). The fiber object
	 *    is just binding to an existing ACL_FIBER. Calling start() is
	 *    prohibited in this mode.
	 *  - false: Fiber needs to be started. You must call start() to begin
	 *    execution. This is the normal mode for creating new fibers.
	 * 
	 * Example:
	 * ```cpp
	 * class MyFiber : public acl::fiber {
	 * public:
	 *     MyFiber() : fiber(false) {}  // Normal mode, need to call start()
	 *     
	 *     void run() override {
	 *         printf("Fiber running\n");
	 *     }
	 * };
	 * 
	 * MyFiber fb;
	 * fb.start();  // Start the fiber
	 * acl::fiber::schedule();  // Run scheduler
	 * ```
	 */
	explicit fiber(bool running);

	/**
	 * Destructor - Destroys the fiber object
	 * 
	 * Note: The fiber should have completed execution before destruction.
	 * Destroying a running fiber may lead to undefined behavior.
	 */
	virtual ~fiber();

	/**
	 * start - Start the fiber execution
	 * 
	 * Starts the fiber by creating its stack and adding it to the scheduler's
	 * ready queue. The run() method will be called when the fiber is scheduled.
	 * 
	 * This method can only be called when the fiber was created with running=false.
	 * Calling start() on an already-running fiber is prohibited.
	 * 
	 * @param stack_size {size_t} The initial stack size for the fiber in bytes.
	 *                   Default: 320000 (320KB). Increase if deep recursion
	 *                   or large local variables are needed.
	 * @param share_stack {bool} Whether to use shared stack mode:
	 *                   - false (default): Each fiber has its own private stack
	 *                   - true: Fibers share a common stack (saves memory but
	 *                     has context switch overhead)
	 * 
	 * Example:
	 * ```cpp
	 * class MyFiber : public acl::fiber {
	 *     void run() override {
	 *         printf("Hello from fiber!\n");
	 *     }
	 * };
	 * 
	 * MyFiber fb;
	 * fb.start(640000);  // Start with 640KB stack
	 * acl::fiber::schedule();
	 * ```
	 */
	void start(size_t stack_size = 320000, bool share_stack = false);

	/**
	 * reset - Reset the fiber handle after completion
	 * 
	 * Sets the fiber's internal handle (f_) to NULL after the fiber has finished.
	 * This method can only be called after the fiber has completed execution.
	 * 
	 * Use case: Allows reusing the fiber object for creating a new fiber.
	 */
	void reset();

	/**
	 * kill - Request the fiber to terminate
	 * 
	 * Sends a termination signal to the fiber. The fiber should check
	 * killed() or self_killed() periodically and exit gracefully.
	 * 
	 * @param sync {bool} Synchronization mode:
	 *             - false (default): Asynchronous kill, returns immediately
	 *             - true: Synchronous kill, waits for the fiber to exit
	 * 
	 * @return {bool} Returns:
	 *         - true: Kill signal sent successfully
	 *         - false: Fiber has not been started or has already exited
	 * 
	 * Example:
	 * ```cpp
	 * class Worker : public acl::fiber {
	 *     void run() override {
	 *         while (!killed()) {  // Check kill signal
	 *             do_work();
	 *             acl::fiber::delay(100);
	 *         }
	 *         printf("Fiber exiting gracefully\n");
	 *     }
	 * };
	 * 
	 * Worker worker;
	 * worker.start();
	 * // ... later ...
	 * worker.kill(true);  // Request termination and wait
	 * ```
	 */
	bool kill(bool sync = false);

	/**
	 * killed - Check if this fiber has been signaled to exit
	 * 
	 * @return {bool} Returns true if kill() has been called on this fiber
	 */
	bool killed() const;

	/**
	 * self_killed - Check if the currently running fiber should exit
	 * 
	 * Static method that checks if the current fiber has received a kill signal.
	 * Fibers should call this periodically and exit gracefully if true.
	 * 
	 * @return {bool} Returns true if the current fiber should exit
	 * 
	 * Example:
	 * ```cpp
	 * void long_running_task() {
	 *     for (int i = 0; i < 1000000; i++) {
	 *         if (acl::fiber::self_killed()) {
	 *             printf("Interrupted at iteration %d\n", i);
	 *             return;  // Exit gracefully
	 *         }
	 *         process_item(i);
	 *     }
	 * }
	 * ```
	 */
	static bool self_killed();

	/**
	 * get_id - Get this fiber's unique ID
	 * 
	 * @return {unsigned int} The fiber's ID number (unique within the thread)
	 */
	unsigned int get_id() const;

	/**
	 * self - Get the currently running fiber's ID
	 * 
	 * Static method that returns the ID of the fiber that is currently executing.
	 * 
	 * @return {unsigned int} The current fiber's ID
	 * 
	 * Example:
	 * ```cpp
	 * void fiber_func() {
	 *     printf("I am fiber %u\n", acl::fiber::self());
	 * }
	 * ```
	 */
	static unsigned int self();

	/**
	 * fiber_id - Get the ID of a specified fiber
	 * 
	 * @param fb {const fiber&} The fiber to query
	 * @return {unsigned int} The fiber's ID number
	 */
	static unsigned int fiber_id(const fiber& fb);

	/**
	 * get_errno - Get this fiber's error number
	 * 
	 * Returns the error number set after a system API call fails.
	 * Each fiber has its own error number (similar to thread-local errno).
	 * 
	 * @return {int} The error number
	 */
	int get_errno() const;

	/**
	 * is_ready - Check if the fiber is in the ready queue
	 * 
	 * @return {bool} Returns true if the fiber is waiting to be scheduled
	 */
	bool is_ready() const;

	/**
	 * is_suspended - Check if the fiber is suspended
	 * 
	 * @return {bool} Returns true if the fiber is in suspended state
	 *                (waiting for I/O, timer, or synchronization primitive)
	 */
	bool is_suspended() const;

	/**
	 * set_errno - Set this fiber's error number
	 * 
	 * @param errnum {int} The error number to set
	 */
	void set_errno(int errnum);

	/**
	 * clear - Clear the current fiber's error flag
	 * 
	 * Resets the fiber's error state.
	 */
	static void clear();

public:
	// ====================================================================
	// Error Handling Methods
	// ====================================================================

	/**
	 * last_serror - Get the last error message string
	 * 
	 * Returns a human-readable error message for the last system API error.
	 * 
	 * @return {const char*} Error message string
	 */
	static const char* last_serror();

	/**
	 * last_error - Get the last error number
	 * 
	 * Returns the error number from the last failed system API call.
	 * 
	 * @return {int} Error number (similar to errno)
	 */
	static int last_error();

	/**
	 * strerror - Convert error number to error message
	 * 
	 * @param errnum {int} The error number to convert
	 * @param buf {char*} Buffer to store the error message
	 * @param size {size_t} Size of the buffer
	 * @return {const char*} Pointer to buf containing the error message
	 */
	static const char* strerror(int errnum, char* buf, size_t size);

	/**
	 * stdout_open - Enable/disable error message output
	 * 
	 * Controls whether internal error messages are printed to stdout.
	 * 
	 * @param on {bool} If true, error messages will be output to stdout
	 */
	static void stdout_open(bool on);

	// ====================================================================
	// System Configuration Methods
	// ====================================================================

	/**
	 * set_fdlimit - Set the maximum number of file descriptors
	 * 
	 * Sets the maximum number of file handles (file descriptors) that
	 * this process can create. Useful for servers handling many connections.
	 * 
	 * @param max {int} The maximum number of file descriptors.
	 *            Only effective when max >= 0.
	 * @return {int} The current maximum number of available file descriptors
	 * 
	 * Example:
	 * ```cpp
	 * // Allow up to 100,000 file descriptors
	 * int limit = acl::fiber::set_fdlimit(100000);
	 * printf("FD limit set to: %d\n", limit);
	 * ```
	 */
	static int set_fdlimit(int max);

	// ====================================================================
	// Scheduler Initialization and Control Methods
	// ====================================================================

	/**
	 * init - Initialize the fiber scheduler with specific settings
	 * 
	 * Explicitly sets the event engine type and optionally enables automatic
	 * scheduler startup. Call this before creating any fibers if you need
	 * specific event engine or auto-scheduling behavior.
	 * 
	 * @param type {fiber_event_t} The event engine to use:
	 *             - FIBER_EVENT_T_KERNEL: Best performance (epoll/kqueue/iocp)
	 *             - FIBER_EVENT_T_POLL: Good compatibility
	 *             - FIBER_EVENT_T_SELECT: Maximum compatibility
	 *             - FIBER_EVENT_T_WMSG: Windows GUI mode
	 *             - FIBER_EVENT_T_IO_URING: Linux io_uring (highest performance)
	 * 
	 * @param schedule_auto {bool} Auto-scheduling mode:
	 *             - false (default): Must explicitly call schedule() to start
	 *             - true: Scheduler starts automatically when first fiber is created
	 * 
	 * Example:
	 * ```cpp
	 * // Use epoll and auto-start scheduler
	 * acl::fiber::init(acl::FIBER_EVENT_T_KERNEL, true);
	 * 
	 * go []() { printf("Hello\n"); };  // Scheduler starts automatically
	 * // No need to call schedule()
	 * ```
	 */
	static void init(fiber_event_t type, bool schedule_auto = false);

	/**
	 * schedule_gui - Enable GUI coroutine mode (Windows only)
	 * 
	 * On the Windows platform, this method enables GUI coroutine mode,
	 * allowing fibers to work with Windows message loops.
	 */
	static void schedule_gui();

	/**
	 * schedule - Start the fiber scheduler
	 * 
	 * Starts the fiber scheduling loop. This method blocks until all fibers
	 * have completed or schedule_stop() is called. This is the main entry
	 * point for running fibers.
	 * 
	 * @param type {fiber_event_t} The event engine to use (default: KERNEL)
	 * 
	 * Example:
	 * ```cpp
	 * go []() { printf("Fiber 1\n"); };
	 * go []() { printf("Fiber 2\n"); };
	 * 
	 * acl::fiber::schedule();  // Blocks until all fibers complete
	 * printf("All fibers done\n");
	 * ```
	 */
	static void schedule(fiber_event_t type = FIBER_EVENT_T_KERNEL);

	/**
	 * schedule_with - Start the scheduler with specified event type
	 * 
	 * Alternative to schedule() that explicitly specifies the event engine.
	 * 
	 * @param type {fiber_event_t} The event engine to use
	 */
	static void schedule_with(fiber_event_t type);

	/**
	 * scheduled - Check if the scheduler is running
	 * 
	 * Checks whether the current thread is in a fiber scheduling state.
	 * 
	 * @return {bool} Returns true if the scheduler is currently running
	 * 
	 * Example:
	 * ```cpp
	 * if (acl::fiber::scheduled()) {
	 *     printf("Scheduler is running\n");
	 * }
	 * ```
	 */
	static bool scheduled();

	/**
	 * schedule_stop - Stop the fiber scheduler
	 * 
	 * Stops the fiber scheduling loop, causing schedule() to return.
	 * Any remaining fibers will not be executed.
	 * 
	 * Example:
	 * ```cpp
	 * go []() {
	 *     acl::fiber::delay(5000);
	 *     acl::fiber::schedule_stop();  // Stop after 5 seconds
	 * };
	 * 
	 * acl::fiber::schedule();  // Will return after 5 seconds
	 * ```
	 */
	static void schedule_stop();

public:
	// ====================================================================
	// Fiber Control and Scheduling Methods
	// ====================================================================

	/**
	 * yield - Voluntarily give up CPU to other fibers
	 * 
	 * The current fiber yields control, allowing other ready fibers to run.
	 * The current fiber is placed back in the ready queue and will be
	 * rescheduled later.
	 * 
	 * Use this in long-running loops to ensure other fibers get CPU time.
	 * 
	 * Example:
	 * ```cpp
	 * void long_computation() {
	 *     for (int i = 0; i < 1000000; i++) {
	 *         compute(i);
	 *         if (i % 1000 == 0) {
	 *             acl::fiber::yield();  // Let other fibers run
	 *         }
	 *     }
	 * }
	 * ```
	 */
	static void yield();

	/**
	 * switch_to_next - Suspend current fiber and run next fiber
	 * 
	 * Suspends the current fiber and immediately executes the next fiber
	 * in the ready queue. Similar to yield() but with more explicit control.
	 */
	static void switch_to_next();

	/**
	 * ready - Add a fiber to the ready queue
	 * 
	 * Appends the specified fiber to the ready queue for execution.
	 * The fiber will be scheduled when it reaches the front of the queue.
	 * 
	 * @param f {fiber&} The fiber to make ready
	 */
	static void ready(fiber& f);

	/**
	 * delay - Sleep the current fiber for specified time
	 * 
	 * Suspends the current fiber for the specified number of milliseconds.
	 * Other fibers continue to run during this time. This is a cooperative
	 * sleep that doesn't block the thread.
	 * 
	 * @param milliseconds {size_t} Number of milliseconds to sleep
	 * @return {size_t} Number of milliseconds remaining if interrupted early
	 *                  (usually 0 if sleep completed normally)
	 * 
	 * Example:
	 * ```cpp
	 * void periodic_task() {
	 *     while (!acl::fiber::self_killed()) {
	 *         do_work();
	 *         acl::fiber::delay(1000);  // Sleep for 1 second
	 *     }
	 * }
	 * ```
	 */
	static size_t delay(size_t milliseconds);

	/**
	 * alive_number - Get the number of active fibers
	 * 
	 * Returns the count of fibers that are currently alive (running,
	 * ready, or suspended) in the current thread.
	 * 
	 * @return {unsigned} Number of active fibers
	 * 
	 * Example:
	 * ```cpp
	 * printf("Active fibers: %u\n", acl::fiber::alive_number());
	 * ```
	 */
	static unsigned alive_number();

	/**
	 * dead_number - Get the number of dead fibers
	 * 
	 * Returns the count of fibers that have finished execution but haven't
	 * been cleaned up yet.
	 * 
	 * @return {unsigned} Number of dead fibers
	 */
	static unsigned dead_number();

	// ====================================================================
	// Advanced Configuration Methods
	// ====================================================================

	/**
	 * set_non_blocking - Configure non-blocking connect behavior (Windows only)
	 * 
	 * Sets all fibers in this thread to use non-blocking mode with timeout
	 * when connecting to servers.
	 * 
	 * @param yes {bool} If true, use non-blocking connect
	 * 
	 * Note: This method can only be used on Windows platform.
	 */
	static void set_non_blocking(bool yes);

	/**
	 * set_shared_stack_size - Set the shared stack size
	 * 
	 * Configures the size of the shared stack when using shared stack mode.
	 * Must be called before creating fibers with share_stack=true.
	 * 
	 * @param size {size_t} The shared stack size in bytes
	 *             Default: 1024000 (1MB)
	 * 
	 * Example:
	 * ```cpp
	 * acl::fiber::set_shared_stack_size(2048000);  // 2MB shared stack
	 * ```
	 */
	static void set_shared_stack_size(size_t size);

	/**
	 * get_shared_stack_size - Get the shared stack size
	 * 
	 * @return {size_t} The shared stack size in bytes.
	 *                  Returns 0 if shared stack mode is not enabled.
	 */
	static size_t get_shared_stack_size();

	/**
	 * set_max_cache - Set maximum number of cached fiber stacks
	 * 
	 * @param max {int} Maximum number of fiber stacks to cache
	 */
	static void set_max_cache(int max);

	/**
	 * get_max_cache - Get maximum number of cached fiber stacks
	 * 
	 * @return {int} Maximum number of cached stacks
	 */
	static int get_max_cache();

	/**
	 * acl_io_hook - Enable I/O hooking
	 * 
	 * Hooks system I/O functions to make them fiber-aware.
	 */
	static void acl_io_hook();

	/**
	 * acl_io_unlock - Disable I/O hooking
	 * 
	 * Unhooks system I/O functions.
	 */
	static void acl_io_unlock();

	/**
	 * winapi_hook - Hook Windows API functions (Windows only)
	 * 
	 * On the Windows platform, this function hooks system APIs related to
	 * network operations to make them fiber-aware.
	 * 
	 * @return {bool} Returns true if hooking succeeded
	 */
	static bool winapi_hook();

	/**
	 * get_sys_errno - Get the system error number
	 * 
	 * @return {int} The system error number (errno)
	 */
	static int  get_sys_errno();

	/**
	 * set_sys_errno - Set the system error number
	 * 
	 * @param errnum {int} The error number to set
	 */
	static void set_sys_errno(int errnum);

	/**
	 * share_epoll - Configure epoll handle sharing (deprecated)
	 * 
	 * Configures whether multiple fibers can share one epoll handle in
	 * the same thread.
	 * 
	 * @param yes {bool} If true, share epoll handle
	 * @deprecated This method is deprecated
	 */
	static void share_epoll(bool yes);

	/**
	 * set_event_directly - Configure direct event delivery
	 * 
	 * @param yes {bool} If true, deliver events directly
	 */
	static void set_event_directly(bool yes);

	/**
	 * set_event_keepio - Configure I/O event persistence
	 * 
	 * @param yes {bool} If true, keep I/O events
	 */
	static void set_event_keepio(bool yes);

	/**
	 * set_event_oneshot - Configure one-shot event mode
	 * 
	 * @param yes {bool} If true, use one-shot event mode
	 */
	static void set_event_oneshot(bool yes);

public:
	// ====================================================================
	// Advanced and Debugging Methods
	// ====================================================================

	/**
	 * get_fiber - Get the underlying C fiber object
	 * 
	 * Returns the corresponding C language fiber object (ACL_FIBER) for
	 * this fiber. Used for interoperability with C code.
	 * 
	 * @return {ACL_FIBER*} Pointer to the underlying C fiber structure
	 */
	ACL_FIBER* get_fiber() const;

	/**
	 * fiber_create - Create a fiber using C API
	 * 
	 * Low-level method to create a fiber using the C API directly.
	 * Most users should use the C++ fiber class or go_fiber macros instead.
	 * 
	 * @param fn {void (*)(ACL_FIBER*, void*)} Fiber entry function
	 * @param ctx {void*} Context pointer passed to the entry function
	 * @param size {size_t} Stack size in bytes
	 * @param share_stack {bool} Whether to use shared stack mode
	 * @return {ACL_FIBER*} Pointer to the created C fiber structure
	 */
	static ACL_FIBER* fiber_create(void (*fn)(ACL_FIBER*, void*),
			void* ctx, size_t size, bool share_stack = false);

	/**
	 * stacktrace - Get the fiber's call stack
	 * 
	 * Retrieves the call stack of the specified fiber for debugging purposes.
	 * 
	 * @param fb {const fiber&} The fiber to trace
	 * @param out {std::vector<fiber_frame>&} Output vector to store stack frames
	 * @param max {size_t} Maximum stack depth to retrieve (default: 50)
	 * 
	 * Example:
	 * ```cpp
	 * std::vector<acl::fiber_frame> frames;
	 * acl::fiber::stacktrace(my_fiber, frames, 20);
	 * for (const auto& frame : frames) {
	 *     printf("%s at PC=%ld\n", frame.func.c_str(), frame.pc);
	 * }
	 * ```
	 */
	static void stacktrace(const fiber& fb, std::vector<fiber_frame>& out,
			size_t max = 50);

	/**
	 * stackshow - Print the fiber's call stack to stdout
	 * 
	 * Prints the call stack of the specified fiber to standard output.
	 * Useful for debugging deadlocks or understanding fiber execution.
	 * 
	 * @param fb {const fiber&} The fiber to trace
	 * @param max {size_t} Maximum stack depth to display (default: 50)
	 * 
	 * Example:
	 * ```cpp
	 * acl::fiber::stackshow(my_fiber);  // Print stack trace
	 * ```
	 */
	static void stackshow(const fiber& fb, size_t max = 50);

protected:
	/**
	 * run - Virtual method to be overridden by subclasses
	 * 
	 * This virtual function must be implemented by subclasses. It contains
	 * the code that will be executed when the fiber runs.
	 * 
	 * This method is called automatically when start() is called (if the
	 * fiber was created with running=false). If running=true in the
	 * constructor, this method will not be called.
	 * 
	 * Example:
	 * ```cpp
	 * class MyFiber : public acl::fiber {
	 * protected:
	 *     void run() override {
	 *         printf("Fiber is running\n");
	 *         // Fiber logic here
	 *     }
	 * };
	 * ```
	 */
	virtual void run();

private:
	ACL_FIBER* f_;  // Underlying C fiber structure

	// Disable copy construction and assignment
	fiber(const fiber&);
	void operator = (const fiber&);

	// Internal callback function
	static void fiber_callback(ACL_FIBER* f, void* ctx);
};

/**
 * fiber_timer - A fiber-based timer class
 * 
 * A coroutine class that can be used as a timer. The run() method is called
 * after the specified delay. Useful for scheduling delayed tasks or periodic
 * operations.
 * 
 * Example:
 * ```cpp
 * class MyTimer : public acl::fiber_timer {
 * protected:
 *     void run() override {
 *         printf("Timer fired!\n");
 *     }
 * };
 * 
 * MyTimer timer;
 * timer.start(5000);  // Fire after 5 seconds
 * acl::fiber::schedule();
 * ```
 */
class FIBER_CPP_API fiber_timer {
public:
	/**
	 * Constructor - Create a fiber timer
	 */
	fiber_timer();
	
	/**
	 * Destructor - Destroys the fiber timer
	 */
	virtual ~fiber_timer() {}

	/**
	 * start - Start the timer
	 * 
	 * Starts the timer fiber. After the specified delay, the run() method
	 * will be called.
	 * 
	 * @param milliseconds {unsigned int} Delay in milliseconds before run() is called
	 * @param stack_size {size_t} The fiber's stack size (default: 320KB)
	 * 
	 * Example:
	 * ```cpp
	 * class DelayedTask : public acl::fiber_timer {
	 * protected:
	 *     void run() override {
	 *         printf("Task executed after delay\n");
	 *     }
	 * };
	 * 
	 * DelayedTask task;
	 * task.start(3000);  // Execute after 3 seconds
	 * ```
	 */
	void start(unsigned int milliseconds, size_t stack_size = 320000);

protected:
	/**
	 * run - Pure virtual method to be implemented by subclasses
	 * 
	 * This method is called when the timer fires (after the specified delay).
	 * Subclasses must implement this method to define what happens when the
	 * timer expires.
	 */
	virtual void run() = 0;

private:
	ACL_FIBER* f_;  // Underlying C fiber structure

	// Disable copy construction and assignment
	fiber_timer(const fiber_timer&);
	void operator = (const fiber_timer&);

	// Internal callback function
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

/**
 * ============================================================================
 * USAGE EXAMPLES
 * ============================================================================
 * 
 * Example 1: Basic Fiber Usage (Class Inheritance)
 * -------------------------------------------------
 * #include "fiber/fiber.hpp"
 * 
 * class MyFiber : public acl::fiber {
 * protected:
 *     void run() override {
 *         printf("Hello from fiber %u\n", get_id());
 *         acl::fiber::delay(1000);
 *         printf("Fiber %u done\n", get_id());
 *     }
 * };
 * 
 * int main() {
 *     MyFiber fb1, fb2, fb3;
 *     fb1.start();
 *     fb2.start();
 *     fb3.start();
 *     
 *     acl::fiber::schedule();  // Run until all fibers complete
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 2: Using go_fiber Macros (Recommended)
 * ============================================================================
 * 
 * #include "fiber/go_fiber.hpp"
 * 
 * void worker(int id) {
 *     printf("Worker %d starting\n", id);
 *     acl::fiber::delay(1000);
 *     printf("Worker %d done\n", id);
 * }
 * 
 * int main() {
 *     // Launch fibers with go macro
 *     go worker(1);
 *     go worker(2);
 *     
 *     // Launch with lambda
 *     go []() {
 *         printf("Lambda fiber\n");
 *     };
 *     
 *     acl::fiber::schedule();
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 3: Producer-Consumer Pattern
 * ============================================================================
 * 
 * #include "fiber/fiber.hpp"
 * #include "fiber/fiber_tbox.hpp"
 * #include <queue>
 * 
 * acl::fiber_tbox<int> mailbox;
 * 
 * class Producer : public acl::fiber {
 * protected:
 *     void run() override {
 *         for (int i = 0; i < 10; i++) {
 *             mailbox.push(new int(i));
 *             printf("Produced: %d\n", i);
 *             acl::fiber::delay(100);
 *         }
 *         mailbox.push(nullptr);  // Termination signal
 *     }
 * };
 * 
 * class Consumer : public acl::fiber {
 * protected:
 *     void run() override {
 *         while (true) {
 *             int* val = mailbox.pop();
 *             if (!val) break;
 *             
 *             printf("Consumed: %d\n", *val);
 *             delete val;
 *         }
 *     }
 * };
 * 
 * int main() {
 *     Producer prod;
 *     Consumer cons;
 *     
 *     prod.start();
 *     cons.start();
 *     
 *     acl::fiber::schedule();
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 4: Fiber Timer Usage
 * ============================================================================
 * 
 * class PeriodicTask : public acl::fiber_timer {
 * protected:
 *     void run() override {
 *         printf("Periodic task executed\n");
 *         // Schedule next execution
 *         start(5000);  // Run again in 5 seconds
 *     }
 * };
 * 
 * int main() {
 *     PeriodicTask task;
 *     task.start(1000);  // First execution after 1 second
 *     
 *     acl::fiber::schedule();
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 5: Graceful Shutdown with kill()
 * ============================================================================
 * 
 * class Worker : public acl::fiber {
 * protected:
 *     void run() override {
 *         while (!killed()) {
 *             printf("Working...\n");
 *             acl::fiber::delay(500);
 *         }
 *         printf("Worker shutting down gracefully\n");
 *     }
 * };
 * 
 * int main() {
 *     Worker worker;
 *     worker.start();
 *     
 *     // Shutdown fiber in another fiber
 *     go [&worker]() {
 *         acl::fiber::delay(3000);  // Wait 3 seconds
 *         worker.kill(true);  // Request shutdown and wait
 *         printf("Worker stopped\n");
 *         acl::fiber::schedule_stop();
 *     };
 *     
 *     acl::fiber::schedule();
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 6: Multiple Event Engines
 * ============================================================================
 * 
 * int main() {
 *     // Use io_uring on Linux for best performance
 *     #ifdef __linux__
 *     acl::fiber::schedule(acl::FIBER_EVENT_T_IO_URING);
 *     #else
 *     acl::fiber::schedule(acl::FIBER_EVENT_T_KERNEL);
 *     #endif
 *     
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 7: Stack Tracing for Debugging
 * ============================================================================
 * 
 * class DebuggableFiber : public acl::fiber {
 * protected:
 *     void run() override {
 *         function_a();
 *     }
 *     
 *     void function_a() {
 *         function_b();
 *     }
 *     
 *     void function_b() {
 *         // Print stack trace
 *         acl::fiber::stackshow(*this);
 *         acl::fiber::delay(1000);
 *     }
 * };
 * 
 * ============================================================================
 * BEST PRACTICES
 * ============================================================================
 * 
 * 1. Fiber Creation:
 *    - Prefer go_fiber macros for simple cases
 *    - Use class inheritance for complex fiber logic
 *    - Set appropriate stack size based on needs
 * 
 * 2. Cooperative Scheduling:
 *    - Call yield() in long-running loops
 *    - Use delay() instead of busy-waiting
 *    - Don't block on system calls (use fiber-aware I/O)
 * 
 * 3. Resource Management:
 *    - Clean up resources in fiber's run() method
 *    - Use RAII for automatic cleanup
 *    - Check killed() for graceful shutdown
 * 
 * 4. Event Engine Selection:
 *    - FIBER_EVENT_T_KERNEL: Best for most cases
 *    - FIBER_EVENT_T_IO_URING: Best on Linux 5.1+
 *    - FIBER_EVENT_T_POLL: Good compatibility
 *    - FIBER_EVENT_T_SELECT: Maximum compatibility
 * 
 * 5. Stack Size:
 *    - Default 320KB is sufficient for most cases
 *    - Increase for deep recursion or large locals
 *    - Use shared stack mode to save memory (with caution)
 * 
 * 6. Synchronization:
 *    - Use fiber_mutex for cross-thread sync
 *    - Use fiber_lock for same-thread sync (faster)
 *    - Use fiber_tbox for message passing
 *    - Use fiber_cond for condition variables
 * 
 * 7. Error Handling:
 *    - Check return values of fiber operations
 *    - Use last_error() and last_serror() for debugging
 *    - Handle fiber termination gracefully
 * 
 * 8. Performance:
 *    - Minimize fiber creation/destruction overhead
 *    - Use fiber pools for high-frequency tasks
 *    - Profile with alive_number() and dead_number()
 *    - Choose appropriate event engine for platform
 * 
 * ============================================================================
 * COMMON PATTERNS
 * ============================================================================
 * 
 * Pattern 1: Fire and Forget
 * ---------------------------
 * go []() {
 *     do_background_work();
 * };
 * 
 * Pattern 2: Wait for Completion
 * -------------------------------
 * acl::wait_group wg;
 * wg.add(3);
 * 
 * go [&wg]() { work(); wg.done(); };
 * go [&wg]() { work(); wg.done(); };
 * go [&wg]() { work(); wg.done(); };
 * 
 * wg.wait();  // Wait for all to complete
 * 
 * Pattern 3: Periodic Task
 * -------------------------
 * go []() {
 *     while (!acl::fiber::self_killed()) {
 *         do_periodic_work();
 *         acl::fiber::delay(1000);  // Every second
 *     }
 * };
 * 
 * Pattern 4: Timeout Pattern
 * ---------------------------
 * acl::fiber_tbox<Result> result_box;
 * 
 * go [&]() {
 *     Result* r = expensive_operation();
 *     result_box.push(r);
 * };
 * 
 * bool found;
 * Result* r = result_box.pop(5000, &found);  // 5 second timeout
 * if (found) {
 *     process(r);
 * } else {
 *     printf("Timeout!\n");
 * }
 * 
 * ============================================================================
 */
