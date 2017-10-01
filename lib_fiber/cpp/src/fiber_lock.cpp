#include "stdafx.hpp"
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
# include <sys/eventfd.h>
# define USE_EVENT
#else
# define USE_PIPE
#endif

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

fiber_mutex::fiber_mutex(bool thread_safe /* = false */,
	unsigned int delay /* = 1000 */)
: delay_(delay)
, waiters_(0)
, readers_(0)
, written_(0)
{
	if (thread_safe)
       	{
		thread_lock_ = new thread_mutex;

#if	defined(USE_EVENT)
		in_ = eventfd(0, 0);
		if (in_ >= 0)
			out_ = in_;
		else
		{
			logger_error("eventfd error %s", last_serror());
			out_ = -1;
		}

#elif	defined(USE_PIPE)
		int fds[2];
		if (acl_duplex_pipe(fds))
		{
			logger_error("duplex pipe error %s", last_serror());
			in_ = out_ = -1;
		}
		else
		{
			in_  = fds[0];
			out_ = fds[1];
		}
#else
		in_ = out_ = -1;
#endif
	}
	else
		thread_lock_ = NULL;

	// sanity check, reset delay_ to 100 ms when in_ less 0
	if ((in_ < 0 || out_ < 0) && delay_ > 100)
		delay_ = 100;

	lock_ = acl_fiber_mutex_create();
}

fiber_mutex::~fiber_mutex(void)
{
	acl_fiber_mutex_free(lock_);
	delete thread_lock_;

	if (in_ >= 0)
		close(in_);
	if (out_ >= 0 && out_ != in_)
		close(out_);
}

bool fiber_mutex::thread_mutex_lock(void)
{
	waiters_++;

	while (thread_lock_->try_lock() == false)
	{
		long long n;

		if (in_ < 0)
		{
			(void) fiber::delay(delay_);
			continue;
		}

		if (acl_read_poll_wait(in_, delay_) == -1)
		{
			if (errno == ACL_ETIMEDOUT)
				continue;

			waiters_--;
			logger_error("read wait error %s", last_serror());
			return false;
		}

		if (written_ == 0 || readers_.cas(0, 1) == 1)
			continue;

		written_--;

		if (read(in_, &n, sizeof(n)) <= 0)
			logger_error("thread-%lu, read error %s",
				acl::thread::thread_self(), last_serror());

		(void) readers_.cas(1, 0);
	}

	waiters_--;
	return true;
}

static __thread int __nfibers = 0;

bool fiber_mutex::lock(void)
{
	if (thread_lock_ && thread_mutex_lock() == false)
		return false;

	if (fiber::scheduled())
	{
		__nfibers++;
		acl_fiber_mutex_lock(lock_);
	}

	return true;
}

bool fiber_mutex::trylock(void)
{
	if (thread_lock_ && !thread_lock_->try_lock())
		return false;

	if (!fiber::scheduled())
		return true;
	else if (acl_fiber_mutex_trylock(lock_) == 0)
	{
		__nfibers++;
		return true;
	}
	else
		return false;
}

bool fiber_mutex::unlock(void)
{
	long long n = 100;

	if (fiber::scheduled())
	{
		__nfibers--;
		acl_fiber_mutex_unlock(lock_);
	}

	if (thread_lock_ && !thread_lock_->unlock())
		return false;

	if (out_ < 0 || __nfibers > 0)
		return true;

#ifdef	USE_EVENT
	if (waiters_ == 0)
#elif	defined(USE_PIPE)
	if (waiters_ == 0 || written_ > 0)
#endif
		return true;

	written_++;

	if (write(out_, &n, sizeof(n)) <= 0)
		logger_warn("write error=%s", last_serror());

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
