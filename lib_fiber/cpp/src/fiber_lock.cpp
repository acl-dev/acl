#include "stdafx.hpp"
#include "fiber/fiber_lock.hpp"

namespace acl {

fiber_lock::fiber_lock()
{
	lock_ = acl_fiber_lock_create();
}

fiber_lock::~fiber_lock()
{
	acl_fiber_lock_free(lock_);
}

bool fiber_lock::lock()
{
	acl_fiber_lock_lock(lock_);
	return true;
}

bool fiber_lock::trylock()
{
	return acl_fiber_lock_trylock(lock_) == 0;
}

bool fiber_lock::unlock()
{
	acl_fiber_lock_unlock(lock_);
	return true;
}

//////////////////////////////////////////////////////////////////////////////

fiber_rwlock::fiber_rwlock()
{
	rwlk_ = acl_fiber_rwlock_create();
}

fiber_rwlock::~fiber_rwlock()
{
	acl_fiber_rwlock_free(rwlk_);
}

void fiber_rwlock::rlock()
{
	acl_fiber_rwlock_rlock(rwlk_);
}

bool fiber_rwlock::tryrlock()
{
	return acl_fiber_rwlock_tryrlock(rwlk_) == 0;
}

void fiber_rwlock::runlock()
{
	acl_fiber_rwlock_runlock(rwlk_);
}

void fiber_rwlock::wlock()
{
	acl_fiber_rwlock_wlock(rwlk_);
}

bool fiber_rwlock::trywlock()
{
	return acl_fiber_rwlock_trywlock(rwlk_) == 0;
}

void fiber_rwlock::wunlock()
{
	acl_fiber_rwlock_wunlock(rwlk_);
}

} // namespace acl
