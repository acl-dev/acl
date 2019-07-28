#include "stdafx.hpp"
#include "fiber/fiber_event.hpp"
#include "fiber/fiber_cond.hpp"

#if !defined(_WIN32) && !defined(_WIN64)

namespace acl {

fiber_cond::fiber_cond(void)
{
	cond_ = acl_fiber_cond_create(0);
}

fiber_cond::~fiber_cond(void)
{
	acl_fiber_cond_free(cond_);
}

bool fiber_cond::wait(fiber_event& event, int timeout /* = -1 */)
{
	ACL_FIBER_EVENT* ev = event.get_event();

	if (timeout < 0) {
		return acl_fiber_cond_wait(cond_, ev) == 0 ? true : false;
	}

	if (acl_fiber_cond_timedwait(cond_, ev, timeout) == 0) {
		return true;
	} else {
		return false;
	}
}

bool fiber_cond::notify(void)
{
	return acl_fiber_cond_signal(cond_) == 0;
}

} // namespace acl

#endif // !defined(_WIN32) && !defined(_WIN64)
