#pragma once
#include "fiber_cpp_define.hpp"

#if defined(USE_CPP11) || __cplusplus >= 201103L

#include <functional>
#include <vector>

namespace acl {

using task_fn = std::function<void(void)>;

template<class task_fn>
class task_box {
public:
	explicit task_box(box2<task_fn>* bx) : box(bx) {}
	~task_box() { delete box; }

	box2<task_fn> *box = nullptr;
	int  idx  = -1;
	int  idle = -1;
};

class wait_group;

class fiber_pool {
public:
	fiber_pool(size_t min, size_t max, int idle_ms = -1, size_t box_buf = 500,
		size_t stack_size = 128000, bool stack_share = false);
	~fiber_pool();

	fiber_pool(const fiber_pool&) = delete;
	fiber_pool& operator=(const fiber_pool&) = delete;

	template<class Fn, class ...Args>
	void exec(Fn&& fn, Args&&... args) {
		auto obj = std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...);
		task_box<task_fn>* box;
		if (box_idle_ > 0) {
			box = boxes_idle_[next_idle_++ % box_idle_];
		} else {
			box = boxes_[next_box_++ % box_count_];
		}

		box->box->push(obj, true);
		if (box_buf_ > 0 && box->box->size() >= (size_t) box_buf_) {
			fiber::yield();
		}
	}

	void stop();

public:
	size_t get_box_min() const {
		return box_min_;
	}

	size_t get_box_max() const {
		return box_max_;
	}

	size_t get_box_count() const {
		return box_count_;
	}

	size_t get_box_idle() const {
		return box_idle_;
	}

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

	size_t box_count_ = 0;
	size_t box_idle_  = 0;
	size_t next_box_  = 0;
	size_t next_idle_ = 0;

	task_box<task_fn> **boxes_;
	task_box<task_fn> **boxes_idle_;
	std::vector<std::shared_ptr<fiber>> fibers_;

	void fiber_create(size_t count);
	void fiber_run(task_box<task_fn>* box);
	void running(task_box<task_fn>* box);
};

} // namespace acl

#endif // __cplusplus >= 201103L
