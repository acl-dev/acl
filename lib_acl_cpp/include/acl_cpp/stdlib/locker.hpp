#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <stdlib.h>
#ifndef WIN32
#include <pthread.h>
#endif

#ifdef WIN32
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
class ACL_CPP_API locker
{
public:
	/**
	 * 构造函数
	 * @param use_mutex {bool} 是否创建线程锁
	 * @param nowait {bool} 是否一直等待至加锁成功
	 *  (包括线程锁和文件锁)
	 */
	locker(bool use_mutex = true, bool nowait = false);
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
#ifdef WIN32
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
	 * 针对已经打开的锁(包括线程锁和文件锁)进行解锁
	 * @return {bool} 解锁是否成功
	 */
	bool unlock();
protected:
private:
	void init_mutex(void);

#ifndef WIN32
	pthread_mutexattr_t  mutexAttr_;
#endif
	acl_pthread_mutex_t* pMutex_;

	char* pFile_;
#ifdef WIN32
	void* fHandle_;
#else
	int   fHandle_;
#endif
	bool  myFHandle_;
	bool  nowait_;
};

}  // namespace acl
