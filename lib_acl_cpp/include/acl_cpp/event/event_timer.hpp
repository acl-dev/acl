#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <list>

namespace acl
{

class event_task;

class ACL_CPP_API event_timer : public noncopyable
{
public:
	/**
	 * 构造函数
	 * @param keep {bool} 该定时器是否允许自动重启
	 */
	event_timer(bool keep = false);
	virtual ~event_timer();

	/**
	 * 当定时器里的任务数为空时的回调函数，
	 * 子类可以在其中释放，一旦该函数被调用，
	 * 则意味着该定时器及其中的所有定时任务都从
	 * 定时器集合中被删除
	 * 该函数被触发的条件有三个：
	 * 1) 定时器所有的任务数为 0 时
	 * 2) 当没有设置重复定时器且该定时器中有一个定时任务被触发后
	 * 3) 当 del_timer(event_timer*) 被调用后
	 */
	virtual void destroy(void) {}

	/**
	 * 定时器里的任务是否为空
	 * @return {bool}
	 */
	bool empty(void) const
	{
		return tasks_.empty();
	}

	/**
	 * 定时器里的任务个数
	 * @return {size_t}
	 */
	size_t length(void) const
	{
		return length_;
	}

	/**
	 * 该定时器是否是自动重启的
	 * @param on {bool}
	 */
	void keep_timer(bool on);

	/**
	 * 判断该定时器是否是自动重启的
	 * @return {bool}
	 */
	bool keep_timer(void) const
	{
		return keep_;
	}

	/**
	 * 清空定时器里的定时任务
	 * @return {int} 被清除的定时任务的个数
	 */
	int clear(void);

	/**
	 * 子类必须实现此回调函数，注：子类或调用者禁止在
	 * timer_callback 内部调用 event_timer 的析构
	 * 函数，否则将会酿成大祸
	 * @param id {unsigned int} 对应某个任务的 ID 号
	 */
	virtual void timer_callback(unsigned int id) = 0;

	/****************************************************************/
	/*        子类可以调用如下函数添加一些新的定时器任务 ID 号      */
	/****************************************************************/
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * 针对本定时器增加新的任务ID号，这样便可以通过一个定时器启动
	 * 多个定时任务
	 * @param id {unsigned int} 定时器定时任务ID号
	 * @param delay {__int64} 每隔多久自动触发该定时器，同时将对应的定时器定时
	 *  任务ID号传回(微秒级)
	 * @return {__int64} 距离本定时器的第一个将会触发的定时任务ID还多久(微秒级)
	 */
	__int64 set_task(unsigned int id, __int64 delay);

	/**
	 * 删除定时器中某个消息ID对应的定时任务
	 * @param {unsigned int} 定时任务ID
	 * @return {__int64} 距离本定时器的第一个将会触发的定时任务ID还多久(微秒级)
	 */
	__int64 del_task(unsigned int id);
#else
	long long int set_task(unsigned int id, long long int delay);
	long long int del_task(unsigned int id);
#endif

	//////////////////////////////////////////////////////////////////////

	/**
	 * 触发所有到时的定时任务
	 * @return {long lont int} 下一个将要到时的任务
	 */
#if defined(_WIN32) || defined(_WIN64)
	__int64 trigger(void);
	__int64 min_delay(void) const
	{
		return min_delay_;
	}
#else
	long long int trigger(void);
	long long int min_delay(void) const
	{
		return min_delay_;
	}
#endif

protected:
#if defined(_WIN32) || defined(_WIN64)
	__int64 min_delay_;
	__int64 present_;
#else
	long long int min_delay_;
	long long int present_;
#endif

private:
	size_t length_;
	std::list<event_task*> tasks_;
	bool keep_;  // 该定时器是否允许自动重启
#if defined(_WIN32) || defined(_WIN64)
	__int64 set_task(event_task* task);
#else
	long long int set_task(event_task* task);
#endif

	/**
	 * 设置当前定时器的时间截
	 */
	void set_time(void);
};

}  // namespace acl
