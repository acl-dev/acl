#pragma once
#include "../acl_cpp_define.hpp"
#include "noncopyable.hpp"

#if !defined(_WIN32) && !defined(_WIN64)
# include <pthread.h>
# ifndef	acl_pthread_mutex_t
#  define	acl_pthread_mutex_t	pthread_mutex_t
# endif
#else
struct acl_pthread_mutex_t;
#endif

namespace acl {

/**
 * 线程互斥锁
 */
class ACL_CPP_API thread_mutex : public noncopyable
{
public:
	/**
	 * 构造方法
	 * @param recursive {bool} 是否启用递归锁方式
	 */
	thread_mutex(bool recursive = true);
	~thread_mutex(void);

	/**
	 * 对线程锁进行加锁，一直到加锁成功或内部失败(一般不会失败，除非是系统问题)
	 * @return {bool} 返回 false 说明线程锁有问题
	 */
	bool lock(void);

	/**
	 * 尝试性加锁，无论成功与否都会立即返回
	 * @return {bool} 返回 true 表示加锁成功，返回 false 表示加锁失败
	 */
	bool try_lock(void);

	/**
	 * 解线程锁
	 * @return {bool} 返回 false 表示解锁失败，有可能之前并未加锁成功所致
	 */
	bool unlock(void);

	/**
	 * 获得 acl 中 C 版本的系统类型的线程锁
	 * @return {acl_pthread_mutex_t*}
	 */
	acl_pthread_mutex_t* get_mutex(void) const;

private:
	acl_pthread_mutex_t* mutex_;
#if !defined(_WIN32) && !defined(_WIN64)
	pthread_mutexattr_t  mutex_attr_;
#endif
};

class ACL_CPP_API thread_mutex_guard : public noncopyable
{
public:
	thread_mutex_guard(thread_mutex& mutex);
	~thread_mutex_guard(void);

private:
	thread_mutex& mutex_;
};

} // namespace acl
