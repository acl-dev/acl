#pragma once
#include "fiber_cpp_define.hpp"

// __cplusplus:
//    199711L (C++98 or C++03)
//    201103L (C++11)
//    201402L (C++14)
//    201703L (C++17)
//    202002L (C++20)

#if defined(USE_CPP11) || __cplusplus >= 201103L      // Support c++11 ?

#include <thread>
#include <functional>
#include <utility>
#include <memory>
#include "fiber.hpp"
#include "fiber_tbox.hpp"

struct ACL_FIBER;

namespace acl {

/**
 * fiber_ctx - Context wrapper for fiber execution
 * 
 * This class wraps a std::function to be executed in a fiber context.
 * It stores the function and transfers ownership to the fiber when created.
 */
class fiber_ctx {
public:
	/**
	 * Constructor - Creates a fiber context with the given function
	 * @param fn The function to be executed in the fiber
	 */
	explicit fiber_ctx(std::function<void()> fn) {
		fn_ = std::move(fn);
	}

	~fiber_ctx() = default;

	std::function<void()> fn_;  // The function to be executed
};

// Macro: go - Start a fiber with default stack size (320000 bytes)
// Usage: go [&] { /* your code */ };
#define	go                  acl::go_fiber()>

// Macro: go_stack - Start a fiber with custom stack size (non-shared)
// Usage: go_stack(size) [&] { /* your code */ };
#define	go_stack(size)      acl::go_fiber(size, false)>

// Macro: go_share - Start a fiber with custom stack size (shared stack)
// Usage: go_share(size) [&] { /* your code */ };
#define	go_share(size)      acl::go_fiber(size, true)>

// Macro: go_wait_fiber - Start a fiber and wait for it to complete
// Usage: go_wait_fiber [&] { /* your code */ };
#define	go_wait_fiber       acl::go_fiber()<

// Macro: go_wait_thread - Start a thread and wait for it to complete
// Usage: go_wait_thread [&] { /* your code */ };
#define	go_wait_thread      acl::go_fiber()<<

// Macro: go_wait - Alias for go_wait_thread
#define	go_wait             go_wait_thread

/**
 * go_fiber - A Go-style fiber launcher for C++
 * 
 * This class provides a convenient way to launch fibers using Go-like syntax.
 * It overloads operators to enable intuitive fiber creation and synchronization.
 * 
 * Features:
 * - operator> : Launch a fiber asynchronously and return a fiber handle
 * - operator< : Launch a fiber and wait for it to complete (fiber context)
 * - operator<<: Launch a thread and wait for it to complete (thread context)
 */
class go_fiber {
public:
	/**
	 * Default constructor - Uses default stack size (320000 bytes, non-shared)
	 */
	go_fiber() = default;

	/**
	 * Constructor with custom stack configuration
	 * @param stack_size The stack size for the fiber
	 * @param on Whether to use shared stack (true) or private stack (false)
	 */
	go_fiber(size_t stack_size, bool on) : stack_size_(stack_size), stack_share_(on) {}

	/**
	 * operator> - Launch a fiber asynchronously
	 * 
	 * Creates and starts a new fiber to execute the given function.
	 * Returns immediately without waiting for the fiber to complete.
	 * 
	 * @param fn The function to execute in the fiber
	 * @return A shared pointer to the created fiber object
	 */
	std::shared_ptr<fiber> operator > (std::function<void()> fn) const {
		auto* ctx = new fiber_ctx(std::move(fn));
		auto fb = fiber::fiber_create(fiber_main, (void*) ctx, stack_size_, stack_share_);
		return std::make_shared<fiber>(fb);
	}

	/**
	 * operator< - Launch a fiber and wait for completion
	 * 
	 * Creates a new fiber, executes the function, and blocks until completion.
	 * Uses a fiber_tbox for synchronization.
	 * 
	 * @param fn The function to execute in the fiber
	 */
	void operator < (std::function<void()> fn) {
		fiber_tbox<int> box;

		go[&] {
			fn();
			box.push(nullptr);
		};
		(void) box.pop();
	}

	/**
	 * operator<< - Launch a thread and wait for completion
	 * 
	 * Creates a new thread, executes the function, and blocks until completion.
	 * Uses a fiber_tbox for synchronization.
	 * 
	 * @param fn The function to execute in the thread
	 */
	void operator << (std::function<void()> fn) {
		fiber_tbox<int> box;

		std::thread thread([&]() {
			fn();
			box.push(nullptr);
		});

		thread.detach();
		(void) box.pop();
	}

private:
	size_t stack_size_  = 320000;  // Default stack size: 320KB
	bool   stack_share_ = false;   // Whether to use shared stack

	/**
	 * fiber_main - Entry point for fiber execution
	 * 
	 * This static function is called when a fiber starts.
	 * It extracts the user function from the context and executes it.
	 * 
	 * @param fiber Pointer to the ACL_FIBER structure (unused)
	 * @param ctx Pointer to the fiber_ctx containing the user function
	 */
	static void fiber_main(ACL_FIBER*, void* ctx) {
		auto* fc = (fiber_ctx *) ctx;
		std::function<void()> fn = fc->fn_;
		delete fc;

		fn();
	}
};

} // namespace acl

#endif // __cplusplus >= 201103L

/**
 * ============================================================================
 * USAGE EXAMPLES
 * ============================================================================
 *
 * Example 1: Simple fiber without parameters
 * -------------------------------------------
 * static void fiber1() {
 * 	printf("fiber: %d\r\n", acl::fiber::self());
 * }
 *
 * Example 2: Fiber with mutable reference parameter
 * --------------------------------------------------
 * static void fiber2(acl::string& buf) {
 * 	printf("in fiber: %d, buf: %s\r\n", acl::fiber::self(), buf.c_str());
 * 	buf = "world";  // Modifies the original buffer
 * }
 *
 * Example 3: Fiber with const reference parameter
 * ------------------------------------------------
 * static void fiber3(const acl::string& buf) {
 * 	printf("in fiber: %d, buf: %s\r\n", acl::fiber::self(), buf.c_str());
 * }
 *
 * Example 4: Helper function for increment
 * -----------------------------------------
 * static void incr(int& n) {
 *	n++;
 * }
 *
 * Example 5: Waiting for fiber/thread completion
 * -----------------------------------------------
 * static void waiter() {
 *	int n = 100;
 *
 *	// Run in a separate thread and wait for result
 *	go_wait_thread[&] { incr(n); };
 *  // Here: n should be 101 (modified by the thread)
 *
 *	n = 200;
 *
 *	// Run in a fiber and wait for result
 *	go_wait_fiber[&] { incr(n); };
 *  // Here: n should be 201 (modified by the fiber)
 * }
 *
 * Example 6: Various fiber launch patterns
 * -----------------------------------------
 * static test() {
 * 	// Launch a fiber without lambda (direct function call)
 * 	go fiber1;
 * 	
 * 	acl::string buf("hello");
 *
 * 	// Launch a fiber with lambda (capture by reference)
 * 	// The fiber can modify 'buf'
 * 	go[&] {
 * 		fiber2(buf);
 * 	};
 * 	
 * 	// Launch a fiber with lambda (capture by value)
 * 	// The fiber gets a copy of 'buf'
 * 	go[=] {
 * 		fiber3(buf);
 * 	};
 * 
 * 	// Launch another fiber with lambda (capture by reference)
 * 	go[&] {
 * 		fiber3(buf);
 * 	};
 * }
 *
 * ============================================================================
 * KEY CONCEPTS
 * ============================================================================
 * 
 * 1. go - Launch fiber asynchronously (fire and forget)
 * 2. go_wait_fiber - Launch fiber and wait for completion
 * 3. go_wait_thread - Launch thread and wait for completion
 * 4. [&] - Capture variables by reference (can modify originals)
 * 5. [=] - Capture variables by value (creates copies)
 * 6. go_stack(size) - Launch fiber with custom stack size
 * 7. go_share(size) - Launch fiber with shared stack
 */
