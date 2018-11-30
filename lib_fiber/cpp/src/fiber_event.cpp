#include "stdafx.hpp"
#include "fiber/fiber_event.hpp"

#if !defined(_WIN32) && !defined(_WIN64)

namespace acl {

fiber_event::fiber_event(bool use_mutex /* = true */,
	bool fatal_on_error /* = true */)
{
	unsigned flag = use_mutex ? FIBER_FLAG_USE_MUTEX : 0;
	if (fatal_on_error) {
		flag |= FIBER_FLAG_USE_FATAL;
	}

	event_ = acl_fiber_event_create(flag);
}

fiber_event::~fiber_event(void)
{
	acl_fiber_event_free(event_);
}

bool fiber_event::wait(void)
{
	return acl_fiber_event_wait(event_) == 0 ? true : false;
}

bool fiber_event::trywait(void)
{
	return acl_fiber_event_trywait(event_) == 0 ? true : false;
}

bool fiber_event::notify(void)
{
	return acl_fiber_event_notify(event_) == 0 ? true : false;
}

} // namespace acl

#endif
