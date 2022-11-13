#pragma once
#include "fiber_cpp_define.hpp"

struct ACL_FIBER_MUTEX;

namespace acl {

/**
 * 可用于同一线程内的协程之间以及不同线程之间的协程之间的互斥锁, 同时还可以用在
 * 线程之间以及协程与独立线程之间的互斥.
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

public:
	/**
	 * 返回 C 版本的互斥锁对象
	 * @return {ACL_FIBER_MUTEX*}
	 */
	ACL_FIBER_MUTEX* get_mutex(void) const
	{
		return mutex_;
	}

private:
	ACL_FIBER_MUTEX* mutex_;

	fiber_mutex(const fiber_mutex&);
	void operator=(const fiber_mutex&);
};

} // namespace acl
