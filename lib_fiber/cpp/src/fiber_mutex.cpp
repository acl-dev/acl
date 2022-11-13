#include "stdafx.hpp"
#include "fiber/fiber_mutex.hpp"

namespace acl {

fiber_mutex::fiber_mutex(void)
{
	mutex_ = acl_fiber_mutex_create(0);
}

fiber_mutex::~fiber_mutex(void)
{
	acl_fiber_mutex_free(mutex_);
}

bool fiber_mutex::lock(void)
{
	acl_fiber_mutex_lock(mutex_);
	return true;
}

bool fiber_mutex::trylock(void)
{
	return acl_fiber_mutex_trylock(mutex_) == 0 ? true : false;
}

bool fiber_mutex::unlock(void)
{
	acl_fiber_mutex_unlock(mutex_);
	return true;
}

} // namespace acl
