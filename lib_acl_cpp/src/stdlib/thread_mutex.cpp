/**
 * Copyright (C) 2017-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Tue 22 Aug 2017 11:13:27 AM CST
 */

#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/thread_mutex.hpp"
#endif

namespace acl {

thread_mutex::thread_mutex(void)
{
	mutex_ = (acl_pthread_mutex_t*)
		acl_mycalloc(1, sizeof(acl_pthread_mutex_t));

#ifdef	ACL_UNIX
	acl_assert(pthread_mutexattr_init(&mutex_attr_) == 0);
	acl_assert(pthread_mutexattr_settype(&mutex_attr_,
			PTHREAD_MUTEX_RECURSIVE) == 0);
	acl_assert(acl_pthread_mutex_init(mutex_, &mutex_attr_) == 0);
#else
	acl_assert(acl_pthread_mutex_init(mutex_, NULL) == 0);
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
	if (ret)
	{
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
	if (ret)
	{
#ifdef ACL_UNIX
		acl_set_error(ret);
		logger_error("pthread_mutex_unlock error %s", last_serror());
#endif
		return false;
	}
	return true;
}

} // namespace acl
