#include "stdafx.hpp"
#include "fiber/fiber_lock.hpp"

namespace acl {

fiber_mutex::fiber_mutex(void)
{
	lock_ = acl_fiber_mutex_create();
}

fiber_mutex::~fiber_mutex(void)
{
	acl_fiber_mutex_free(lock_);
}

void fiber_mutex::lock(void)
{
	acl_fiber_mutex_lock(lock_);
}

bool fiber_mutex::trylock(void)
{
	return acl_fiber_mutex_trylock(lock_) == 0 ? false : true;
}

void fiber_mutex::unlock(void)
{
	acl_fiber_mutex_unlock(lock_);
}

//////////////////////////////////////////////////////////////////////////////

fiber_rwlock::fiber_rwlock(void)
{
	rwlk_ = acl_fiber_rwlock_create();
}

fiber_rwlock::~fiber_rwlock(void)
{
	acl_fiber_rwlock_free(rwlk_);
}

void fiber_rwlock::rlock(void)
{
	acl_fiber_rwlock_rlock(rwlk_);
}

bool fiber_rwlock::tryrlock(void)
{
	return acl_fiber_rwlock_tryrlock(rwlk_) == 0 ? false : true;
}

void fiber_rwlock::runlock(void)
{
	acl_fiber_rwlock_runlock(rwlk_);
}

void fiber_rwlock::wlock(void)
{
	acl_fiber_rwlock_wlock(rwlk_);
}

bool fiber_rwlock::trywlock(void)
{
	return acl_fiber_rwlock_trywlock(rwlk_) == 0 ? false : true;
}

void fiber_rwlock::wunlock(void)
{
	acl_fiber_rwlock_wunlock(rwlk_);
}

} // namespace acl
