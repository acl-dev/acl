#include "stdafx.hpp"

#if defined(USE_CPP11) || __cplusplus >= 201103L

#include <cassert>
#include <utility>
#include "fiber/fiber.hpp"
#include "fiber/go_fiber.hpp"
#include "fiber/wait_group.hpp"
#include "fiber/fiber_sem.hpp"
#include "fiber/fiber_pool.hpp"

namespace acl {

fiber_pool::fiber_pool(size_t min, size_t max, int idle_ms, size_t box_buf,
	size_t stack_size, bool stack_share)
: idle_ms_(idle_ms)
, box_buf_(box_buf)
, stack_size_(stack_size)
, stack_share_(stack_share)
{
	assert(max >= min && min > 0);
	assert(stack_size > 0);

	box_min_    = min;
	box_max_    = max;
	boxes_      = new task_box<task_fn>* [max];
	boxes_idle_ = new task_box<task_fn>* [max];
	wg_         = new wait_group;

	fiber_create(min);
}

fiber_pool::~fiber_pool()
{
	for (size_t i = 0; i < box_count_; i++) {
		delete boxes_[i];
	}

	delete []boxes_;
	delete []boxes_idle_;
	delete wg_;
}

void fiber_pool::stop()
{
	for (const auto& fb : fibers_) {
		fb->kill();
	}

	wg_->wait();
}

void fiber_pool::fiber_create(size_t count)
{
	for (size_t i = 0; i < count; i++) {
		auto* box2 = new fiber_sbox2<task_fn>(box_buf_);
		auto* box  = new task_box<task_fn>(box2);

		boxes_[box_count_] = box;
		box->idx = (int) box_count_++;

		boxes_idle_[box_idle_] = box;
		box->idle = (int) box_idle_++;

		wg_->add(1);

		if (stack_share_) {
			auto fb = go_share(stack_size_)[this, box] {
				fiber_run(box);
				delete box;
			};
			fibers_.push_back(fb);
		} else {
			auto fb = go_stack(stack_size_)[this, box] {
				fiber_run(box);
				delete box;
			};
			fibers_.push_back(fb);
		}
	}
}

void fiber_pool::fiber_run(task_box<task_fn>* box)
{
	running(box);

	if (box_count_-- > 1) {
		boxes_[box->idx] = boxes_[box_count_];
		boxes_[box->idx]->idx = box->idx;
		boxes_[box_count_] = nullptr;
	} else {
		assert(box_count_ == 0);
		boxes_[box_count_] = nullptr;
	}

	if (box->idle >= 0) {
		if (box_idle_-- > 1) {
			boxes_idle_[box->idle] = boxes_idle_[box_idle_];
			boxes_idle_[box->idle]->idle = box->idle;
			boxes_idle_[box_idle_] = nullptr;
		} else {
			assert(box_idle_ == 0);
			assert(boxes_idle_[0] == box);
			boxes_idle_[0] = nullptr;
		}
	}

	wg_->done();
}

void fiber_pool::running(task_box<task_fn>* box)
{
	while (true) {
		task_fn t;

		if (!box->box->pop(t, idle_ms_)) {
			if (fiber::self_killed()) {
				break;
			} else if (last_error() == EAGAIN) {
				if (box_count_ > box_min_) {
					break;
				}
				continue;
			}
		}

		if (box->idle >= 0) {
			if (box_idle_-- > 1) {
				boxes_idle_[box->idle] = boxes_idle_[box_idle_];
				boxes_idle_[box->idle]->idle = box->idle;
				boxes_idle_[box_idle_] = nullptr;
			} else {
				assert(box_idle_ == 0);
				assert(boxes_idle_[0] == box);
				boxes_idle_[0] = nullptr;
			}

			box->idle = -1;
		}

		if (box_idle_ == 0 && box_count_ < box_max_) {
			fiber_create(1);
		}

		t();

		assert(box_idle_ < box_count_);

		box->idle = (int) box_idle_;
		boxes_idle_[box_idle_++] = box;
	}
}

} // namespace acl

#endif // __cplusplus >= 201103L
