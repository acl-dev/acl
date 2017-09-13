/**
 * Copyright (C) 2017-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Tue 22 Aug 2017 11:24:57 AM CST
 */

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
	if (mutex)
	{
		mutex_internal_ = NULL;
		mutex_ = mutex;
	}
	else
	{
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

bool thread_cond::wait(long long microseconds /* = -1 */)
{
#define	SEC_TO_NS	1000000000	// nanoseconds per second
#define SEC_TO_MIS	1000000		// microseconds per second
#define MIS_TO_NS	1000		// nanoseconds per microseconds

	acl_pthread_mutex_t* mutex = mutex_->get_mutex();
	if (mutex_->lock() == false)
	{
		logger_error("lock error!");
		return false;
	}

	if (microseconds < 0)
	{
		int  ret1 = acl_pthread_cond_wait(cond_, mutex) ;
		if (ret1)
		{
#ifdef ACL_UNIX
			acl_set_error(ret1);
#endif
			logger_error("pthread_cond_wait error %s",
				last_serror());
		}

		bool ret2 = mutex_->unlock();
		if (!ret2)
			logger_error("mutex unlock error");
		return ret1 == 0 && ret2;
	}

	struct timeval tv;
	gettimeofday(&tv, NULL);

	struct timespec ts;
	ts.tv_sec   = tv.tv_sec + microseconds / SEC_TO_MIS;
	long long n = (tv.tv_usec + microseconds % SEC_TO_MIS) * MIS_TO_NS;
	ts.tv_nsec  = n % SEC_TO_NS;
	ts.tv_sec  += n / SEC_TO_NS;

	int  ret1 = acl_pthread_cond_timedwait(cond_, mutex, &ts);
	if (ret1)
	{
#ifdef ACL_UNIX
		acl_set_error(ret1);
#endif
		logger_error("pthread_cond_timedwait error %s", last_serror());
	}

	bool ret2 = mutex_->unlock();
	if (!ret2)
		logger_error("mutex unlock error");
	return ret1 == 0 && ret2;
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
