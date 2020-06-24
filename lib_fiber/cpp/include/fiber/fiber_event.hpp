#pragma once
#include "fiber_cpp_define.hpp"

#if !defined(_WIN32) && !defined(_WIN64)

struct ACL_FIBER_EVENT;

namespace acl {

/**
 * 可用于协程之间、线程之间以及协程与线程之间，通过事件等待/通知方式进行同步的
 * 的事件混合锁
 */
class FIBER_CPP_API fiber_event
{
public:
	/**
	 * 构造方法
	 * @param use_mutex {bool} 在用在多线程之间进行事件同步时，如果启动的
	 *  的线程数较多（成百上千个线程），则此标志应设为 true 以便于内部在
	 *  同步内部对象时使用线程互斥锁进行保护，以避免形成惊群现象，如果启动
	 *  的线程数较多但该标志为 false，则内部使用原子数进行同步保护，很容易
	 *  造成惊群问题；当启动的线程数较（几十个左右），则此参数可以设为 false
	 *  以告之内部使用原子数进行同步保护
	 * @param fatal_on_error {bool} 内部发生错误时是否直接崩溃，以便于开发
	 *  人员进行错误调试
	 */
	fiber_event(bool use_mutex = true, bool fatal_on_error = true);
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

public:
	/**
	 * 返回 C 版本的事件对象
	 * @return {ACL_FIBER_EVENT*}
	 */
	ACL_FIBER_EVENT* get_event(void) const
	{
		return event_;
	}

private:
	ACL_FIBER_EVENT* event_;

	fiber_event(const fiber_event&);
	void operator=(const fiber_event&);
};

} // namespace acl

#endif

