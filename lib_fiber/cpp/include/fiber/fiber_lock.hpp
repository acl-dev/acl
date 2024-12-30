#pragma once
#include "fiber_cpp_define.hpp"

struct ACL_FIBER_LOCK;
struct ACL_FIBER_RWLOCK;

namespace acl {

/**
 * 仅能用于同一线程内部的协程之间进行互斥的互斥锁
 */
class FIBER_CPP_API fiber_lock {
public:
	fiber_lock();
	~fiber_lock();

	/**
	 * 等待互斥锁
	 * @return {bool} 返回 true 表示加锁成功，否则表示内部出错
	 */
	bool lock();

	/**
	 * 尝试等待互斥锁
	 * @return {bool} 返回 true 表示加锁成功，否则表示锁正在被占用
	 */
	bool trylock();

	/**
	 * 互斥锁拥有者释放锁并通知等待者
	 * @return {bool} 返回 true 表示通知成功，否则表示内部出错
	 */
	bool unlock();

private:
	ACL_FIBER_LOCK* lock_;

	fiber_lock(const fiber_lock&);
	void operator=(const fiber_lock&);
};

/**
 * 仅能用在同一线程内的协程之间进行互斥的读写锁
 */
class FIBER_CPP_API fiber_rwlock {
public:
	fiber_rwlock();
	~fiber_rwlock();

	/**
	 * 加读锁
	 */
	void rlock();

	/**
	 * 尝试加读锁
	 * @return {bool} 返回 true 表示加锁成功，否则表示锁正在被占用
	 */
	bool tryrlock();

	/**
	 * 解读锁
	 */
	void runlock();

	/**
	 * 加写锁
	 */
	void wlock();

	/**
	 * 尝试加写锁
	 * @return {bool} 返回 true 表示加锁成功，否则表示锁正在被占用
	 */
	bool trywlock();

	/**
	 * 解写锁
	 */
	void wunlock();

private:
	ACL_FIBER_RWLOCK* rwlk_;

	fiber_rwlock(const fiber_rwlock&);
	void operator=(const fiber_rwlock&);
};

} // namespace acl
