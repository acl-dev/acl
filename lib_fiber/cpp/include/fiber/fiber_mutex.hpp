#pragma once
#include <vector>
#include "fiber_cpp_define.hpp"
#include "fiber_mutex_stat.hpp"

struct ACL_FIBER_MUTEX;

namespace acl {

/**
 * 可用于同一线程内的协程之间以及不同线程之间的协程之间的互斥锁, 同时还可以用在
 * 线程之间以及协程与独立线程之间的互斥.
 */
class FIBER_CPP_API fiber_mutex
{
public:
	/**
	 * 构造函数
	 * @param mutex {ACL_FIBER_MUTEX*} 非空时,将用 C 的锁对象创建 C++ 锁
	 *  对象,否则内部自动创建 C 锁对象;如果非空时,在本对象析构时该传入的
	 *  C 锁对象需由应用层自行释放.
	 */
	fiber_mutex(ACL_FIBER_MUTEX* mutex = NULL);
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

public:
	/**
	 * 返回 C 版本的互斥锁对象
	 * @return {ACL_FIBER_MUTEX*}
	 */
	ACL_FIBER_MUTEX* get_mutex(void) const
	{
		return mutex_;
	}

	/**
	 * 进行全局死锁检测
	 * @param out {fiber_mutex_stats&} 存储结果集
	 * @return {bool} 返回 true 表示存在死锁问题, 死锁信息存放在 out 中
	 */
	static bool deadlock(fiber_mutex_stats& out);

	/**
	 * 检测死锁, 并将所有进入死锁状态的协程栈打印至标准输出
	 */
	static void deadlock_show(void);

private:
	ACL_FIBER_MUTEX* mutex_;
	ACL_FIBER_MUTEX* mutex_internal_;

	fiber_mutex(const fiber_mutex&);
	void operator=(const fiber_mutex&);
};

class FIBER_CPP_API fiber_mutex_guard
{
public:
	fiber_mutex_guard(fiber_mutex& mutex) : mutex_(mutex) {
		mutex_.lock();
	}

	~fiber_mutex_guard(void) {
		mutex_.unlock();
	}

private:
	fiber_mutex& mutex_;

};

} // namespace acl
