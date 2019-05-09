#pragma once
#include "../acl_cpp_define.hpp"
#include "noncopyable.hpp"
#include <stdlib.h>
#if !defined(_WIN32) && !defined(_WIN64)
#include <pthread.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
struct acl_pthread_mutex_t;
#else
# ifndef	acl_pthread_mutex_t
#  define	acl_pthread_mutex_t	pthread_mutex_t
# endif
#endif

namespace acl {

/**
 * 互斥锁，可以同时创建文件锁和线程锁，也可以只创建一种锁
 */
class ACL_CPP_API locker : public noncopyable
{
public:
	/**
	 * 构造函数
	 * @param use_mutex {bool} 是否创建线程锁
	 * @param use_spinlock {bool} 内部当使用线程锁时是否需要自旋锁
	 */
	locker(bool use_mutex = true, bool use_spinlock = false);
	virtual ~locker();

	/**
	 * 根据文件路径创建文件锁
	 * @param file_path {const char*} 文件路径，非空
	 * @return {bool} 是否成功
	 * 注：此函数与下面的 open 函数仅能同时调用一个
	 */
	bool open(const char* file_path);

	/**
	 * 根据文件句柄创建文件锁
	 * @param fh {int} 文件句柄
	 * @return {bool} 是否成功
	 */
#if defined(_WIN32) || defined(_WIN64)
	bool open(void* fh);
#else
	bool open(int fh);
#endif

	/**
	 * 针对已经打开的锁(包括线程锁和文件锁)进行加锁
	 * @return {bool} 加锁是否成功
	 */
	bool lock();

	/**
	 * 尝试对已经打开的锁(包括线程锁和文件锁)进行加锁
	 * @return {bool} 加锁是否成功
	 */
	bool try_lock();

	/**
	 * 针对已经打开的锁(包括线程锁和文件锁)进行解锁
	 * @return {bool} 解锁是否成功
	 */
	bool unlock();

private:
	acl_pthread_mutex_t* mutex_;
	char* pFile_;
#if defined(_WIN32) || defined(_WIN64)
	void* fHandle_;
#else
	int   fHandle_;
	pthread_mutexattr_t  mutex_attr_;
# if !defined(MINGW) && !defined(__APPLE__) && !defined(ANDROID)
	pthread_spinlock_t*  spinlock_;
# endif
#endif
	bool  myFHandle_;

	void init_mutex(bool use_spinlock);
};

class ACL_CPP_API lock_guard : public noncopyable
{
public:
	lock_guard(locker& lk);
	~lock_guard();

private:
	locker& lk_;
};

}  // namespace acl
