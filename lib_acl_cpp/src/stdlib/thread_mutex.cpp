#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/thread_mutex.hpp"
#endif

#undef	SET_ERRNO
#ifdef	ACL_WINDOWS
# define	SET_ERRNO(_x_) (void) 0
#elif	defined(ACL_UNIX)
# define	SET_ERRNO(_x_) (acl_set_error(_x_))
#else
# error "unknown OS type"
#endif

namespace acl {

thread_mutex::thread_mutex(bool recursive /* = true */)
{
	mutex_ = (acl_pthread_mutex_t*)
		acl_mycalloc(1, sizeof(acl_pthread_mutex_t));

#ifdef	ACL_UNIX
	int ret = pthread_mutexattr_init(&mutex_attr_);
	if (ret) {
		SET_ERRNO(ret);
		logger_fatal("pthread_mutexattr_init error=%s", last_serror());
	}
	if (recursive && (ret = pthread_mutexattr_settype(&mutex_attr_,
					PTHREAD_MUTEX_RECURSIVE))) {
		SET_ERRNO(ret);
		logger_fatal("pthread_mutexattr_settype error=%s", last_serror());
	}
	ret = acl_pthread_mutex_init(mutex_, &mutex_attr_);
	if (ret) {
		SET_ERRNO(ret);
		logger_fatal("pthread_mutex_init error=%s", last_serror());
	}
#else
	(void) recursive;
	int ret = acl_pthread_mutex_init(mutex_, NULL);
	if (ret) {
		SET_ERRNO(ret);
		logger_fatal("pthread_mutex_init error=%s", last_serror());
	}
#endif
}

thread_mutex::~thread_mutex(void)
{
#ifndef	ACL_WINDOWS
	(void) pthread_mutexattr_destroy(&mutex_attr_);
#endif
	(void) acl_pthread_mutex_destroy(mutex_);
	acl_myfree(mutex_);
}

acl_pthread_mutex_t* thread_mutex::get_mutex(void) const
{
	return mutex_;
}

bool thread_mutex::lock(void)
{
	int ret = acl_pthread_mutex_lock(mutex_);
	if (ret) {
#ifdef ACL_UNIX
		acl_set_error(ret);
		logger_error("pthread_mutex_lock error %s", last_serror());
#endif
		return false;
	}
	return true;
}

bool thread_mutex::try_lock(void)
{
	return acl_pthread_mutex_trylock(mutex_) == 0;
}

bool thread_mutex::unlock(void)
{
	int ret = acl_pthread_mutex_unlock(mutex_);
	if (ret) {
#ifdef ACL_UNIX
		acl_set_error(ret);
		logger_error("pthread_mutex_unlock error %s", last_serror());
#endif
		return false;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////

thread_mutex_guard::thread_mutex_guard(thread_mutex& mutex)
: mutex_(mutex)
{
	if (!mutex_.lock()) {
		logger_fatal("lock error=%s", last_serror());
	}
}

thread_mutex_guard::~thread_mutex_guard(void)
{
	if (!mutex_.unlock()) {
		logger_fatal("unlock error=%s", last_serror());
	}
}

} // namespace acl
