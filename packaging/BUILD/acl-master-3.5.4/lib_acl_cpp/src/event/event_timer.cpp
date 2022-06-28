#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/event/event_timer.hpp"
#endif

namespace acl
{

//////////////////////////////////////////////////////////////////////////

class event_task
{
public:
	event_task(void) {}
	~event_task(void) {}

private:
	friend class event_timer;

	unsigned int id;
	acl_int64 delay;
	acl_int64 when;
};

//////////////////////////////////////////////////////////////////////////

event_timer::event_timer(bool keep /* = false */)
{
	keep_ = keep;
	length_ = 0;
	min_delay_ = 2147483647;
}

event_timer::~event_timer(void)
{
	(void) clear();
}

int event_timer::clear(void)
{
	int  n = 0;
	std::list<event_task*>::iterator it = tasks_.begin();
	for (; it != tasks_.end(); ++it) {
		delete (*it);
		n++;
	}
	tasks_.clear();
	length_ = 0;
	return n;
}

void event_timer::keep_timer(bool on)
{
	keep_ = on;
}

void event_timer::set_time(void)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	present_ = ((acl_uint64) now.tv_sec) * 1000000
		+ ((acl_uint64) now.tv_usec);
}

#define TIMER_EMPTY		-1

acl_int64 event_timer::del_task(unsigned int id)
{
	bool ok = false;
	std::list<event_task*>::iterator it = tasks_.begin();
	for (; it != tasks_.end(); ++it) {
		if ((*it)->id == id) {
			delete (*it);
			tasks_.erase(it);
			length_--;
			ok = true;
			break;
		}
	}

	if (!ok) {
		logger_warn("timer id: %u not found", id);
	}

	if (tasks_.empty()) {
		return TIMER_EMPTY;
	}

	set_time();

	event_task* first = tasks_.front();
	acl_int64 delay = first->when - present_;

	if (delay < 0) {
		return 0;
	} else if (delay > first->delay) { /* xxx */
		return first->delay;
	} else {
		return delay;
	}
}

acl_int64 event_timer::set_task(unsigned int id, acl_int64 delay)
{
	if (delay < 0) {
		logger_error("invalid task, id: %u, delay: %lld", id, delay);
		return -1;
	}

	event_task* task = NULL;
	std::list<event_task*>::iterator it = tasks_.begin();
	for (; it != tasks_.end(); ++it) {
		if ((*it)->id == id) {
			task = (*it);
			tasks_.erase(it);
			length_--;
			break;
		}
	}

	if (task == NULL) {
		task = NEW event_task();
		task->delay = delay;
		task->id = id;
	} else {
		task->delay = delay;
	}

	return set_task(task);
}

acl_int64 event_timer::set_task(event_task* task)
{
	set_time();
	task->when = present_ + task->delay;

	if (task->delay < min_delay_) {
		min_delay_ = task->delay;
	}

	std::list<event_task*>::iterator it = tasks_.begin();
	for (; it != tasks_.end(); ++it) {
		if (task->when < (*it)->when) {
			tasks_.insert(it, task);
			break;
		}
	}

	if (it == tasks_.end()) {
		tasks_.push_back(task);
	}

	length_++;

	event_task* first = tasks_.front();
	acl_int64 delay = first->when - present_;

	if (delay < 0) {
		return 0;
	} else if (delay > first->delay) { /* xxx */
		return first->delay;
	} else {
		return delay;
	}
}

acl_int64 event_timer::trigger(void)
{
	// sanity check
	if (tasks_.empty()) {
		return TIMER_EMPTY;
	}

	acl_assert(length_ > 0);

	set_time();

	std::list<event_task*> tasks;

	// 从定时器中取出到达的定时任务
	for (std::list<event_task*>::iterator it = tasks_.begin();
		it != tasks_.end();) {

		if ((*it)->when > present_) {
			break;
		}

		tasks.push_back(*it);
		it = tasks_.erase(it);
		length_--;
	}

	if (tasks.empty()) {
		acl_assert(!tasks_.empty());

		event_task* first = tasks_.front();
		acl_int64 delay = first->when - present_;
		return delay < 0 ? 0 : delay;
	}

	for (std::list<event_task*>::iterator it = tasks.begin();
		it != tasks.end(); ++it) {

		set_task(*it);
		// 调用子类虚函数，触发定时器任务过程
		timer_callback((*it)->id);
	}

	tasks.clear();

	// 子类有可能会在 timer_callback 中删除了所有的定时任务
	if (tasks_.empty()) {
		return TIMER_EMPTY;
	}

	event_task* first = tasks_.front();
	acl_int64 delay = first->when - present_;

	if (delay < 0) {
		return 0;
	} else if (delay > first->delay) { /* xxx */
		return first->delay;
	} else {
		return delay;
	}
}

}  // namespace acl
