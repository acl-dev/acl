#include "stdafx.hpp"
#include "fiber/fiber_tbox.hpp"
#include "fiber/wait_group.hpp"

namespace acl {

wait_group::wait_group(void)
{
	state_ = 0;
	box_   = new acl::fiber_tbox<unsigned long>;
}

wait_group::~wait_group(void)
{
	delete box_;
}

void wait_group::add(int n)
{
	long long state = state_.add_fetch((long long)n << 32);

	//高32位为任务数量
	int c = (int)(state >> 32);

	//低32位为等待者数量
	unsigned w =  (unsigned)state;

	//count不能小于0
	if (c < 0){
		logger_fatal("Negative wait_group counter");
	}

	if (w != 0 && n > 0 && c == n){
		logger_fatal("Add called concurrently with wait");
	}

	if (c > 0 || w == 0) {
		return;
	}

	//检查state是否被修改
	if (state_ != state) {
		logger_fatal("Add called concurrently with wait");
	}

	//这里count为0了，清空state并唤醒所有等待者
	state_ = 0;

	for (size_t i = 0; i < w; i++) {
#ifdef	_DEBUG
		unsigned long* tid = new unsigned long;
		*tid = acl::thread::self();
		box_->push(tid);
#else
		box_->push(NULL);
#endif
	}
}

void wait_group::done(void)
{
	add(-1);
}

void wait_group::wait(void)
{
	for(;;) {
		long long state = state_;
		int c = (int) (state >> 32);

		//没有任务直接返回
		if (c == 0) {
			return;
		}

		//等待者数量加一，失败的话重新获取state
		if (state_.cas(state, state + 1) == state) {
			bool found;
#ifdef	_DEBUG
			unsigned long* tid = box_->pop(-1, &found);
			assert(found);
			delete tid;
#else
			(void) box_->pop(-1, &found);
			assert(found);
#endif
			if(state_ == 0) {
				return;
			}

			logger_fatal("Reused before previous wait has returned");
		}
	}
}

} // namespace acl
