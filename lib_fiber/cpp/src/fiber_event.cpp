#include "stdafx.hpp"
#include "fiber/fiber_event.hpp"

namespace acl {

fiber_event::fiber_event(void)
{
	event_ = acl_fiber_event_create();
}

fiber_event::~fiber_event(void)
{
	acl_fiber_event_free(event_);
}

bool fiber_event::wait(void)
{
	if (acl_fiber_event_wait(event_) == -1)
		return false;

	return true;
}

bool fiber_event::trywait(void)
{
	return acl_fiber_event_trywait(event_) == 0 ? true : false;
}

bool fiber_event::notify(void)
{
	if (acl_fiber_event_notify(event_) == 0)
		return true;
	return false;
}

} // namespace acl
