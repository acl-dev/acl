#include "stdafx.hpp"
#include "fiber/fiber_lock.hpp"

namespace acl {

fiber_lock::fiber_lock(void)
{
	lock_ = acl_fiber_lock_create();
}

fiber_lock::~fiber_lock(void)
{
	acl_fiber_lock_free(lock_);
}

bool fiber_lock::lock(void)
{
	acl_fiber_lock_lock(lock_);
	return true;
}

bool fiber_lock::trylock(void)
{
	return acl_fiber_lock_trylock(lock_) == 0 ? true : false;
}

bool fiber_lock::unlock(void)
{
	acl_fiber_lock_unlock(lock_);
	return true;
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
