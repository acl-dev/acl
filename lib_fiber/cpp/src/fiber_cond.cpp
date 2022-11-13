#include "stdafx.hpp"
#include "fiber/fiber_mutex.hpp"
#include "fiber/fiber_cond.hpp"

namespace acl {

fiber_cond::fiber_cond(void)
{
	cond_ = acl_fiber_cond_create(0);
}

fiber_cond::~fiber_cond(void)
{
	acl_fiber_cond_free(cond_);
}

bool fiber_cond::wait(fiber_mutex& mutex, int timeout /* = -1 */)
{
	ACL_FIBER_MUTEX* m = mutex.get_mutex();

	if (timeout < 0) {
		return acl_fiber_cond_wait(cond_, m) == 0 ? true : false;
	}

	if (acl_fiber_cond_timedwait(cond_, m, timeout) == 0) {
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

