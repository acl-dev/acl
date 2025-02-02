#pragma once
#include "fiber_cpp_define.hpp"
#include <thread>
#include <functional>
#include <utility>
#include "fiber.hpp"
#include "fiber_tbox.hpp"

// __cplusplus:
//    199711L (C++98 or C++03)
//    201103L (C++11)
//    201402L (C++14)
//    201703L (C++17)
//    202002L (C++20)

#if defined(USE_CPP11) || __cplusplus >= 201103L      // Support c++11 ?

struct ACL_FIBER;

namespace acl {

class fiber_ctx {
public:
	explicit fiber_ctx(std::function<void()> fn) {
		fn_ = std::move(fn);
	}

	~fiber_ctx() = default;

	std::function<void()> fn_;
};

#define	go                  acl::go_fiber()>
#define	go_stack(size)      acl::go_fiber(size, false)>
#define	go_share(size)      acl::go_fiber(size, true)>

#define	go_wait_fiber       acl::go_fiber()<
#define	go_wait_thread      acl::go_fiber()<<
#define	go_wait             go_wait_thread

class go_fiber {
public:
	go_fiber() = default;
	go_fiber(size_t stack_size, bool on) : stack_size_(stack_size), stack_share_(on) {}

	std::shared_ptr<fiber> operator > (std::function<void()> fn) const {
		auto* ctx = new fiber_ctx(std::move(fn));
		auto fb = fiber::fiber_create(fiber_main, (void*) ctx, stack_size_, stack_share_);
		return std::make_shared<fiber>(fb);
	}

	void operator < (std::function<void()> fn) {
		fiber_tbox<int> box;

		go[&] {
			fn();
			box.push(nullptr);
		};
		(void) box.pop();
	}

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
	size_t stack_size_  = 320000;
	bool   stack_share_ = false;

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
 * static void fiber1() {
 * 	printf("fiber: %d\r\n", acl::fiber::self());
 * }
 *
 * static void fiber2(acl::string& buf) {
 * 	printf("in fiber: %d, buf: %s\r\n", acl::fiber::self(), buf.c_str());
 * 	buf = "world";
 * }
 *
 * static void fiber3(const acl::string& buf) {
 * 	printf("in fiber: %d, buf: %s\r\n", acl::fiber::self(), buf.c_str());
 * }
 *
 * static void incr(int& n) {
 *	n++;
 * }
 *
 * static void waiter() {
 *	int n = 100;
 *
 *	// run in thread and wait for result
 *	go_wait_thread[&] { incr(n); };
 *  // here: n should be 101
 *
 *	n = 200;
 *
 *	// run in fiber and wait for result
 *	go_wait_fiber[&] { incr(n); };
 *  // here: n should be 201
 * }
 *
 * static test() {
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
