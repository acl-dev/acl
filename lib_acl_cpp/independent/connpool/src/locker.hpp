#pragma once
#include <stdlib.h>
#ifndef WIN32
#include <pthread.h>
#endif

#ifdef WIN32
# error "win32 not support!"
#endif

namespace acl_min
{

/**
 * 互斥锁，可以同时创建文件锁和线程锁，也可以只创建一种锁
 */
class locker
{
public:
	locker(bool nowait = false);
	virtual ~locker();

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

private:
	pthread_mutexattr_t  mutexAttr_;
	pthread_mutex_t* pMutex_;
	bool  nowait_;

	void init_mutex(void);
};

}  // namespace acl_min
