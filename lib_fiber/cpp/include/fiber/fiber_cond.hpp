#pragma once
#include "fiber_cpp_define.hpp"

#if !defined(_WIN32) && !defined(_WIN64)

struct ACL_FIBER_COND;

namespace acl {

class fiber_event;

/**
 * 可用在协程之间，线程之间，协程与线程之间的条件变量
 */
class FIBER_CPP_API fiber_cond
{
public:
	fiber_cond(void);
	~fiber_cond(void);

	/**
	 * 等待条件变量事件被触发
	 * @param event {fiber_event&}
	 * @param timeout {int} 超时等待时间（毫秒）
	 * @return {bool} 成功时返回 true，否则返回 false 表示超时
	 */
	bool wait(fiber_event& event, int timeout = -1);

	/**
	 * 唤醒在条件变量上的等待者，如果没有等待者则直接返回，运行行为和
	 * 线程条件变量类似
	 * @return {bool} 成功返回 true，否则返回 false 表示失败
	 */
	bool notify(void);

public:
	/**
	 * 返回 C 版本的条件变量对象
	 * @return {ACL_FIBER_COND*}
	 */
	ACL_FIBER_COND* get_cond(void) const
	{
		return cond_;
	}

private:
	ACL_FIBER_COND* cond_;

	fiber_cond(const fiber_cond&);
	void operator=(const fiber_cond&);
};

}

#endif
