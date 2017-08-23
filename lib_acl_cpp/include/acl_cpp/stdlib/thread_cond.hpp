/**
 * Copyright (C) 2017-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Tue 22 Aug 2017 11:20:34 AM CST
 */

#pragma once
#include "../acl_cpp_define.hpp"
#if !defined(_WIN32) && !defined(_WIN64)
# include <pthread.h>
# ifndef	acl_pthread_cond_t
#  define	acl_pthread_cond_t	pthread_cond_t
# endif
#else
struct acl_pthread_cond_t;
#endif

namespace acl {

class thread_mutex;

class ACL_CPP_API thread_cond
{
public:
	thread_cond(void);
	thread_cond(thread_mutex& mutex);
	~thread_cond(void);

	bool notify(void);
	bool notify_all(void);
	bool wait(long long microseconds = 0);

	thread_mutex& get_mutex(void) const;
	acl_pthread_cond_t* get_cond(void) const;

private:
	thread_mutex* mutex_;
	thread_mutex* mutex_internal_;
	acl_pthread_cond_t*  cond_;
};

} // namespace acl
