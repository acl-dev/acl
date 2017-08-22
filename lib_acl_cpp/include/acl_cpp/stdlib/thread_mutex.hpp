/**
 * Copyright (C) 2017-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Tue 22 Aug 2017 11:09:55 AM CST
 */

#pragma once
#include "../acl_cpp_define.hpp"

#if !defined(_WIN32) && !defined(_WIN64)
# include <pthread.h>
# ifndef	acl_pthread_mutex_t
#  define	acl_pthread_mutex_t	pthread_mutex_t
# endif
#else
struct acl_pthread_mutex_t;
#endif

namespace acl {

class ACL_CPP_API thread_mutex
{
public:
	thread_mutex(void);
	~thread_mutex(void);

	bool lock(void);
	bool try_lock(void);
	bool unlock(void);

	acl_pthread_mutex_t* get_mutex(void) const;

private:
	acl_pthread_mutex_t* mutex_;
#if !defined(_WIN32) && !defined(_WIN64)
	pthread_mutexattr_t  mutex_attr_;
#endif
};

} // namespace acl
