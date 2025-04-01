#pragma once
#include "fiber_cpp_define.hpp"

#if defined(USE_CPP11) || __cplusplus >= 201103L

#include <functional>
#include <memory>
#include <set>
#include "fiber.hpp"
#include "fiber_sem.hpp"

namespace acl {

/**
 * @brief The task function type.
 * @note The task function should be a callable object, such as a lambda
 * function, a function pointer.
 */
using task_fn = std::function<void(void)>;

/**
 * @brief The task box class for holding tasks to be run by the fiber.
 * @tparam task_fn The type of the task function.
 */
template<class task_fn>
class task_box {
public:
	explicit task_box(fiber_sbox2<task_fn>* bx) : box(bx) {}
	~task_box() { delete box; }

	std::shared_ptr<fiber> fb;
	fiber_sbox2<task_fn> *box = nullptr;
	ssize_t idx  = -1;
	ssize_t idle = -1;
};

class wait_group;

using fibers_set = std::set<std::shared_ptr<fiber>, std::owner_less<std::shared_ptr<fiber>>>;

/**
 * @brief The fiber pool class for running tasks in fibers; One fiber owns one
 * task box, and the task box is used to hold the tasks to be run by the fiber.
 */
class fiber_pool {
public:
	/**
	 * @brief Construct a new fiber pool object.
	 * @param min The minimum number of fibers in the pool which can be 0.
	 * @param max The maximum number of fibers in the pool which must be
	 *  greater or equal than min: when min is 0, max must > min, and when
	 *  min > 0, then max can >= min.
	 * @param idle_ms The idle time in milliseconds before a idle fiber exiting.
	 * @param box_buf The buffer size of the task box.
	 * @param stack_size The size of the stack for each fiber.
	 * @param stack_share The flag indicating whether the fibers' stack is shared.
	 */
	fiber_pool(size_t min, size_t max, int idle_ms = -1, size_t box_buf = 500,
		size_t stack_size = 128000, bool stack_share = false);
	~fiber_pool();

	fiber_pool(const fiber_pool&) = delete;
	fiber_pool& operator=(const fiber_pool&) = delete;

	/**
	 * @brief Execute a task in the fiber pool.
	 * @param fn The function to be run by one fiber of the fiber pool.
	 * @param args The args to be passed to the function.
	 */
	template<class Fn, class ...Args>
	void exec(Fn&& fn, Args&&... args) {
		auto obj = std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...);
		task_box<task_fn>* box;
		if (box_idle_ > 0) {
			box = boxes_idle_[box_idle_ - 1];
		} else if (box_count_ < box_max_) {
			fiber_create(1);
			box = boxes_[box_next_++ % box_count_];
		} else {
			box = boxes_[box_next_++ % box_count_];
		}

		box->box->push(obj, true);
		if (box_buf_ > 0 && box->box->size() >= (size_t) box_buf_) {
			fiber::yield();
		}
	}

	/**
	 * @brief Stop the fiber pool.
	 */
	void stop();

public:
	/**
	 * @brief Get the minimum size of the fiber pool.
	 * @return size_t 
	 */
	size_t get_box_min() const {
		return box_min_;
	}

	/**
	 * @brief Get the maximum size of the fiber pool.
	 * @return size_t 
	 */
	size_t get_box_max() const {
		return box_max_;
	}

	/**
	 * @brief Get the count of fibers in the pool.
	 * @return size_t 
	 */
	size_t get_box_count() const {
		return box_count_;
	}

	/**
	 * @brief Get the idle count of idle fibers in the fiber pool.
	 * @return size_t 
	 */
	size_t get_box_idle() const {
		return box_idle_;
	}

	/**
	 * @brief Get the box buf of holding objects.
	 * @return size_t 
	 */
	size_t get_box_buf() const {
		return box_buf_;
	}

private:
	wait_group* wg_;
	int    idle_ms_;
	size_t box_buf_;
	size_t stack_size_;
	bool   stack_share_;

	size_t box_min_;
	size_t box_max_;

	size_t box_count_  = 0;
	size_t box_next_   = 0;
	ssize_t box_idle_  = 0;

	task_box<task_fn> **boxes_;
	task_box<task_fn> **boxes_idle_;
	fibers_set fibers_;

	void fiber_create(size_t count);
	void fiber_run(task_box<task_fn>* box);
	void running(task_box<task_fn>* box);
};

/**
 * Sample usage:
 * void mytest(acl::wait_group& wg, int i) {
 *    printf("Task %d is running\n", i);
 *    wg.done();
 * }
 *
 * int main(int argc, char* argv[]) {
 *    acl::fiber_pool pool(1, 20, 60, 500, 64000, false);
 *    acl::wait_group wg;
 *    int i = 0;
 *
 *    wg.add(1);
 *    pool.exec([&wg, i]() {
 *        printf("Task %d is running\n", i);
 *        wg.done();
 *    });
 *    i++;
 *
 *    wg.add(1);
 *    pool.exec([&wg](int i) {
 *  	  printf("Task %d is running\n", i);
 *  	  wg.done();
 *    }, i);
 *    i++;
 *
 *    wg.add(1);
 *    pool.exec(mytest, std::ref(wg), i);
 *
 *    go[&wg, &pool] {
 *        wg.wait();
 *        pool.stop();
 *    };
 *
 *    acl::fiber::schedule();
 *    return 0;
 * }
 */
} // namespace acl

#endif // __cplusplus >= 201103L
