#include "stdafx.hpp"
#include "fiber/fiber_event.hpp"

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

fiber_event::~fiber_event()
{
	acl_fiber_event_free(event_);
}

bool fiber_event::wait()
{
	return acl_fiber_event_wait(event_) == 0;
}

bool fiber_event::trywait()
{
	return acl_fiber_event_trywait(event_) == 0;
}

bool fiber_event::notify()
{
	return acl_fiber_event_notify(event_) == 0;
}

} // namespace acl

