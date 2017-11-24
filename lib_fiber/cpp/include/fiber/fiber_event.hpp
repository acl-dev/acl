#pragma once

struct ACL_FIBER_EVENT;

namespace acl {

/**
 * 可用于协程之间、线程之间以及协程与线程之间，通过事件等待/通知方式进行同步的
 * 的事件混合锁
 */
class fiber_event
{
public:
	fiber_event(void);
	~fiber_event(void);

	/**
	 * 等待事件锁
	 * @return {bool} 返回 true 表示加锁成功，否则表示内部出错
	 */
	bool wait(void);

	/**
	 * 尝试等待事件锁
	 * @return {bool} 返回 true 表示加锁成功，否则表示锁正在被占用
	 */
	bool trywait(void);

	/**
	 * 事件锁拥有者释放事件锁并通知等待者
	 * @return {bool} 返回 true 表示通知成功，否则表示内部出错
	 */
	bool notify(void);

private:
	ACL_FIBER_EVENT* event_;
};

} // namespace acl
