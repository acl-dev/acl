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

/**
 * 线程条件变量
 */
class ACL_CPP_API thread_cond
{
public:
	/**
	 * 构造方法
	 * @param mutex {thread_mutex*} 当该参数非 NULL 时，内部自动引用该线程锁，
	 *  否则，内部创建线程锁
	 */
	thread_cond(thread_mutex* mutex = NULL);
	~thread_cond(void);

	/**
	 * 等待线程条件变量就绪
	 * @param microseconds {long long} 等待条件变量就绪的超时时间(微秒级)
	 *   > 0 时表示等待超时的时间
	 *   == 0，不等待
	 *   < 0 则一直等待直到条件变量就绪
	 * @return {bool} 返回 true 表示条件变量就绪，否则表示超时或没被通知
	 */
	bool wait(long long microseconds = -1);

	/**
	 * 通知一个或几个等待在线程条件变量上的线程，表示条件变量就结
	 * @return {bool} 返回 false 表示通知失败
	 */
	bool notify(void);

	/**
	 * 通知所有等待在线程条件变量上的线程，表示条件变量就结
	 * @return {bool} 返回 false 表示通知失败
	 */
	bool notify_all(void);

	/**
	 * 获得与该线程条件变量绑定的线程互斥锁
	 * @return {thread_mutex&}
	 */
	thread_mutex& get_mutex(void) const;

	/**
	 * 获得系统类型的线程条件变量对象
	 * @return {acl_pthread_cond_t*}
	 */
	acl_pthread_cond_t* get_cond(void) const;

private:
	thread_mutex* mutex_;
	thread_mutex* mutex_internal_;
	acl_pthread_cond_t*  cond_;
};

} // namespace acl
