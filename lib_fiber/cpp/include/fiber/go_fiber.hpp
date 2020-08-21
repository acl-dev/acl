#pragma once
#include "fiber_cpp_define.hpp"
#include <thread>
#include <functional>
#include "fiber.hpp"
#include "fiber_tbox.hpp"

struct ACL_FIBER;

namespace acl {

class FIBER_CPP_API fiber_ctx
{
public:
	fiber_ctx(std::function<void()> fn) {
		fn_ = std::move(fn);
	}

	~fiber_ctx() = default;

	std::function<void()> fn_;
};

#define	go			acl::go_fiber()>
#define	go_stack(size)		acl::go_fiber(size)>

#define	go_wait_fiber		acl::go_fiber()<
#define	go_wait_thread		acl::go_fiber()<<
#define	go_wait			go_wait_thread

class FIBER_CPP_API go_fiber
{
public:
	go_fiber(void) {}
	go_fiber(size_t stack_size) : stack_size_(stack_size) {}

	void operator > (std::function<void()> fn)
	{
		fiber_ctx* ctx = new fiber_ctx(fn);
		fiber::fiber_create(fiber_main, (void*) ctx, stack_size_);
	}

	void operator < (std::function<void()> fn)
	{
		fiber_tbox<int> box;

		go[&] {
			fn();
			box.push(NULL);
		};
		(void) box.pop();
	}

	void operator << (std::function<void()> fn)
	{
		fiber_tbox<int> box;

		std::thread thread([&]() {
			fn();
			box.push(NULL);
		});

		thread.detach();
		(void) box.pop();
	}

private:
	size_t stack_size_ = 320000;

	static void fiber_main(ACL_FIBER*, void* ctx)
	{
		fiber_ctx* fc = (fiber_ctx *) ctx;
		std::function<void()> fn = fc->fn_;
		delete fc;

		fn();
	}
};

} // namespace acl

/**
 * static void fiber1(void)
 * {
 * 	printf("fiber: %d\r\n", acl::fiber::self());
 * }
 *
 * static void fiber2(acl::string& buf)
 * {
 * 	printf("in fiber: %d, buf: %s\r\n", acl::fiber::self(), buf.c_str());
 * 	buf = "world";
 * }
 *
 * static void fiber3(const acl::string& buf)
 * {
 * 	printf("in fiber: %d, buf: %s\r\n", acl::fiber::self(), buf.c_str());
 * }
 *
 * static void incr(int& n)
 * {
 *	n++;
 * }
 *
 * static void waiter(void)
 * {
 *	int n = 100;
 *
 *	// run in thread and wait for result
 *	go_wait_thread[&] { incr(n); };
 *      // here: n should be 101
 *
 *	n = 200;
 *
 *	// run in fiber and wait for result
 *	go_wait_fiber[&] { incr(n); };
 *      // here: n should be 201
 * }
 *
 * static test(void)
 * {
 * 	go fiber1;
 * 	
 * 	acl::string buf("hello");
 *
 * 	go[&] {
 * 		fiber2(buf);
 * 	};
 * 	
 * 	go[=] {
 * 		fiber3(buf);
 * 	};
 * 
 * 	go[&] {
 * 		fiber3(buf);
 * 	};
 * }
 */
