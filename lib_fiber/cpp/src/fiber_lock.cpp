#include "stdafx.hpp"
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
# include <sys/eventfd.h>
# define USE_EVENT
#else
# define USE_PIPE
#endif

//#undef   USE_EVENT
//#undef   USE_PIPE

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
	unsigned int delay /* = 1000 */, bool use_atomic_lock /* = true */)
: delay_(delay)
, waiters_(0)
, readers_(0)
, written_(0)
{
	if (thread_safe)
       	{
		if (use_atomic_lock)
		{
			atomic_lock_ = new atomic_long(0);
			thread_lock_ = NULL;
		}
		else
		{
			thread_lock_ = new thread_mutex;
			atomic_lock_ = NULL;
		}

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
	{
		atomic_lock_ = NULL;
		thread_lock_ = NULL;
		in_ = out_ = -1;
	}

	// sanity check, reset delay_ to 100 ms when in_ less 0
	if ((in_ < 0 || out_ < 0) && delay_ > 100)
		delay_ = 100;

	lock_ = acl_fiber_mutex_create();
}

fiber_mutex::~fiber_mutex(void)
{
	acl_fiber_mutex_free(lock_);
	delete atomic_lock_;
	delete thread_lock_;

	if (in_ >= 0)
		close(in_);
	if (out_ >= 0 && out_ != in_)
		close(out_);
}

bool fiber_mutex::lock_wait(int in)
{
	if (in < 0)
	{
		(void) fiber::delay(delay_);
		return true;
	}

	if (acl_read_poll_wait(in, delay_) == -1)
	{
		if (errno == ACL_ETIMEDOUT)
			return true;

		logger_error("read wait error %s", last_serror());
		return false;
	}

	if (written_ == 0 || readers_.cas(0, 1) != 0)
		return true;

	written_--;

	long long n;
	if (read(in, &n, sizeof(n)) <= 0)
		logger_error("thread-%lu, read error %s",
			acl::thread::self(), last_serror());

	if (readers_.cas(1, 0) != 1)
		logger_fatal("thread-%lu, cas invalid", acl::thread::self());

	return true;
}

static __thread int __nfibers = 0;

bool fiber_mutex::lock(void)
{
	if (atomic_lock_)
	{
		waiters_++;

		while (atomic_lock_->cas(0, 1) != 0)
		{
			int in = in_ >= 0 ? dup(in_) : -1;
			if (lock_wait(in) == false)
			{
				waiters_--;
				if (in >= 0)
					close(in);
				return false;
			}
			if (in >= 0)
				close(in);
		}

		waiters_--;
	}
	else if (thread_lock_)
	{
		waiters_++;

		while (thread_lock_->try_lock() == false)
		{
			int in = in_ >= 0 ? dup(in_) : -1;
			if (lock_wait(in) == false)
			{
				waiters_--;
				if (in >= 0)
					close(in);
				return false;
			}
			if (in >= 0)
				close(in);
		}

		waiters_--;
	}

	if (fiber::scheduled())
	{
		__nfibers++;
		acl_fiber_mutex_lock(lock_);
	}

	return true;
}

bool fiber_mutex::trylock(void)
{
	if (atomic_lock_)
	{
		if (atomic_lock_->cas(0, 1) != 1)
			return false;
	}
	else if (thread_lock_)
	{
		if (!thread_lock_->try_lock())
			return false;
	}

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
	if (fiber::scheduled())
	{
		__nfibers--;
		acl_fiber_mutex_unlock(lock_);
	}

	if (atomic_lock_)
	{
		if (atomic_lock_->cas(1, 0) != 1)
		{
			logger_error("cas invalid");
			return false;
		}
	}
	else if (thread_lock_)
	{
		if (!thread_lock_->unlock())
		{
			logger_error("unlock error");
			return false;
		}
	}

	if (out_ < 0 || __nfibers > 0)
		return true;

#ifdef	USE_EVENT
	if (waiters_ == 0)
#elif	defined(USE_PIPE)
	if (waiters_ == 0 || written_ > 0)
#endif
		return true;

	written_++;

	static const long long n = 1;

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
