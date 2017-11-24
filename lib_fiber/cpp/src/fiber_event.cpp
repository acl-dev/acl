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
