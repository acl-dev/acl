#pragma once
#include "fiber_cpp_define.hpp"

struct ACL_FIBER_MUTEX;
struct ACL_FIBER_RWLOCK;

namespace acl {

/**
 * 仅能用于同一线程内部的协程之间进行互斥的互斥锁
 */
class FIBER_CPP_API fiber_mutex
{
public:
	fiber_mutex(void);
	~fiber_mutex(void);

	/**
	 * 等待互斥锁
	 * @return {bool} 返回 true 表示加锁成功，否则表示内部出错
	 */
	bool lock(void);

	/**
	 * 尝试等待互斥锁
	 * @return {bool} 返回 true 表示加锁成功，否则表示锁正在被占用
	 */
	bool trylock(void);

	/**
	 * 互斥锁拥有者释放锁并通知等待者
	 * @return {bool} 返回 true 表示通知成功，否则表示内部出错
	 */
	bool unlock(void);

private:
	ACL_FIBER_MUTEX* lock_;

	fiber_mutex(const fiber_mutex&);
	void operator=(const fiber_mutex&);
};

/**
 * 仅能用在同一线程内的协程之间进行互斥的读写锁
 */
class FIBER_CPP_API fiber_rwlock
{
public:
	fiber_rwlock(void);
	~fiber_rwlock(void);

	/**
	 * 加读锁
	 */
	void rlock(void);

	/**
	 * 尝试加读锁
	 * @return {bool} 返回 true 表示加锁成功，否则表示锁正在被占用
	 */
	bool tryrlock(void);

	/**
	 * 解读锁
	 */
	void runlock(void);

	/**
	 * 加写锁
	 */
	void wlock(void);

	/**
	 * 尝试加写锁
	 * @return {bool} 返回 true 表示加锁成功，否则表示锁正在被占用
	 */
	bool trywlock(void);

	/**
	 * 解写锁
	 */
	void wunlock(void);

private:
	ACL_FIBER_RWLOCK* rwlk_;

	fiber_rwlock(const fiber_rwlock&);
	void operator=(const fiber_rwlock&);
};

} // namespace acl
