#include "stdafx.hpp"
#include "acl_cpp/stdlib/thread.hpp"
#include "acl_cpp/stdlib/thread_mutex.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/atomic.hpp"
#include "acl_cpp/stdlib/mbox.hpp"
#include "acl_cpp/stdlib/trigger.hpp"
#include "fiber/fiber.hpp"
#include "fiber/fiber_lock.hpp"

namespace acl {

fiber_mutex::fiber_mutex(void)
: tid_(0)
{
	lock_ = acl_fiber_event_create();
}

fiber_mutex::~fiber_mutex(void)
{
	acl_fiber_event_free(lock_);
}

bool fiber_mutex::lock(void)
{
	if (acl_fiber_event_wait(lock_) == -1)
		return false;

	tid_ = thread::self();
	return true;
}

bool fiber_mutex::unlock(void)
{
	unsigned long tid = tid_;
	tid_ = 0;
	if (acl_fiber_event_signal(lock_) == 0)
		return true;
	// xxx
	tid_ = tid;
	return false;
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
