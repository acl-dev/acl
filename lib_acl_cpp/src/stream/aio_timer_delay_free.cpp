#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stream/aio_delay_free.hpp"
#endif
#include "aio_timer_delay_free.hpp"

namespace acl
{

#define DELAY_TIMER_ID	0

aio_timer_delay_free::aio_timer_delay_free(aio_handle& handle)
: handle_(handle)
{
}

aio_timer_delay_free::~aio_timer_delay_free()
{
	std::set<aio_delay_free*>::iterator it;
	it = gc_set_.begin();
	for (; it != gc_set_.end(); ++it)
	{
		if (!(*it)->locked())
			(*it)->destroy();
		else
			logger_error("one timer locked yet!");
	}

	gc_set_.clear();
}

void aio_timer_delay_free::timer_callback(unsigned int /* id */)
{
	std::set<aio_delay_free*>::iterator it, next;
	for (it = gc_set_.begin(); it != gc_set_.end(); it = next)
	{
		next = it;
		++next;
		if (!(*it)->locked())
		{
			(*it)->destroy();
			gc_set_.erase(it);
		}
	}

	// 不管事件引擎是否设置了重复定时器过程，重置本定时器任务
	if (!gc_set_.empty())
		handle_.set_timer(this, 100000, DELAY_TIMER_ID);
	else
		handle_.del_timer(this, DELAY_TIMER_ID);
}

bool aio_timer_delay_free::add(aio_delay_free* callback)
{
	std::set<aio_delay_free*>::iterator it = gc_set_.find(callback);
	if (it != gc_set_.end())
		return false;
	gc_set_.insert(callback);
	return true;
}

bool aio_timer_delay_free::del(aio_delay_free* callback)
{
	std::set<aio_delay_free*>::iterator it =
		gc_set_.find(callback);
	if (it != gc_set_.end())
	{
		gc_set_.erase(it);
		return true;
	}
	else
		return false;
}

} // namespace acl
