#include "stdafx.hpp"
#include "fiber/fiber_lock.hpp"

namespace acl {

fiber_mutex::fiber_mutex(void)
{
	lock_ = fiber_mutex_create();
}

fiber_mutex::~fiber_mutex(void)
{
	fiber_mutex_free(lock_);
}

void fiber_mutex::lock(void)
{
	::fiber_mutex_lock(lock_);
}

bool fiber_mutex::trylock(void)
{
	return ::fiber_mutex_trylock(lock_) == 0 ? false : true;
}

void fiber_mutex::unlock(void)
{
	::fiber_mutex_unlock(lock_);
}

//////////////////////////////////////////////////////////////////////////////

fiber_rwlock::fiber_rwlock(void)
{
	rwlk_ = fiber_rwlock_create();
}

fiber_rwlock::~fiber_rwlock(void)
{
	fiber_rwlock_free(rwlk_);
}

void fiber_rwlock::rlock(void)
{
	::fiber_rwlock_rlock(rwlk_);
}

bool fiber_rwlock::tryrlock(void)
{
	return ::fiber_rwlock_tryrlock(rwlk_) == 0 ? false : true;
}

void fiber_rwlock::runlock(void)
{
	::fiber_rwlock_runlock(rwlk_);
}

void fiber_rwlock::wlock(void)
{
	::fiber_rwlock_wlock(rwlk_);
}

bool fiber_rwlock::trywlock(void)
{
	return ::fiber_rwlock_trywlock(rwlk_) == 0 ? false : true;
}

void fiber_rwlock::wunlock(void)
{
	::fiber_rwlock_wunlock(rwlk_);
}

} // namespace acl
