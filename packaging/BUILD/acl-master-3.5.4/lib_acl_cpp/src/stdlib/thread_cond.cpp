#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/thread_mutex.hpp"
#include "acl_cpp/stdlib/thread_cond.hpp"
#endif

namespace acl {

thread_cond::thread_cond(thread_mutex* mutex)
{
	if (mutex) {
		mutex_internal_ = NULL;
		mutex_ = mutex;
	} else {
		mutex_internal_ = NEW thread_mutex;
		mutex_  = mutex_internal_;
	}

	cond_ = (acl_pthread_cond_t*)
		acl_mycalloc(1, sizeof(acl_pthread_cond_t));
	acl_pthread_cond_init(cond_, NULL);
}

thread_cond::~thread_cond(void)
{
	acl_pthread_cond_destroy(cond_);
	acl_myfree(cond_);
	delete mutex_internal_;
}

bool thread_cond::notify(void)
{
	return acl_pthread_cond_signal(cond_) == 0;
}

bool thread_cond::notify_all(void)
{
	return acl_pthread_cond_broadcast(cond_) == 0;
}

#define	SEC_TO_NS	1000000000	// nanoseconds per second
#define SEC_TO_MIS	1000000		// microseconds per second
#define MIS_TO_NS	1000		// nanoseconds per microseconds

bool thread_cond::wait(long long microseconds /* = -1 */,
	bool locked /* = false */)
{
	if (microseconds >= 0) {
		return timed_wait(microseconds, locked);
	} else {
		return block_wait(locked);
	}
}

bool thread_cond::block_wait(bool locked)
{
	bool locked_internal;

	if (!locked) {
		if (!mutex_->lock()) {
			logger_error("lock error=%s", last_serror());
			return false;
		}
		locked_internal = true;
	} else {
		locked_internal = false;
	}

	int ret = acl_pthread_cond_wait(cond_, mutex_->get_mutex());
	if (ret) {
#ifdef ACL_UNIX
		acl_set_error(ret);
#endif
		logger_error("pthread_cond_wait error %s", last_serror());
	}

	// 如果本方法内部前面加了锁，则此处需要解锁
	if (locked_internal && !mutex_->unlock()) {
		logger_error("mutex unlock error=%s", last_serror());
		return false;
	}

	return ret == 0 ? true : false;
}

bool thread_cond::timed_wait(long long microseconds, bool locked)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct timespec ts;
	ts.tv_sec   = (time_t) (tv.tv_sec + microseconds / SEC_TO_MIS);
	long long n = (tv.tv_usec + microseconds % SEC_TO_MIS) * MIS_TO_NS;
	ts.tv_nsec  = (long) n % SEC_TO_NS;
	ts.tv_sec  += (long) n / SEC_TO_NS;

	bool locked_internal;
	if (mutex_internal_ || !locked) {
		if (!mutex_->lock()) {
			logger_error("lock error=%s", last_serror());
			return false;
		}
		locked_internal = true;
	} else {
		locked_internal = false;
	}

	int ret = acl_pthread_cond_timedwait(cond_, mutex_->get_mutex(), &ts);
	if (ret) {
#ifdef ACL_UNIX
		acl_set_error(ret);
#endif
		if (ret != ACL_ETIMEDOUT) {
			logger_error("pthread_cond_timedwait error=%s",
				last_serror());
		}
	}

	if (locked_internal && !mutex_->unlock()) {
		logger_error("mutex unlock error=%s", last_serror());
		return false;
	}

	return ret == 0 ? true : false;
}

thread_mutex& thread_cond::get_mutex(void) const
{
	return *mutex_;
}

acl_pthread_cond_t* thread_cond::get_cond(void) const
{
	return cond_;
}

} // namespace acl
