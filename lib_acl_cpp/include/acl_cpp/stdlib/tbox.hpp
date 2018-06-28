#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include "thread_mutex.hpp"
#include "thread_cond.hpp"
#include "noncopyable.hpp"

namespace acl
{

/**
 * 用于线程之间的消息通信，通过线程条件变量及线程锁实现
 *
 * 示例：
 *
 * class myobj
 * {
 * public:
 *     myobj(void) {}
 *     ~myobj(void) {}
 *
 *     void test(void) { printf("hello world\r\n"); }
 * };
 *
 * acl::tbox<myobj> tbox;
 *
 * void thread_producer(void)
 * {
 *     myobj* o = new myobj;
 *     tbox.push(o);
 * }
 *
 * void thread_consumer(void)
 * {
 *     myobj* o = tbox.pop();
 *     o->test();
 *     delete o;
 * }
 */

template<typename T>
class tbox : public noncopyable
{
public:
	tbox(void) : size_(0) , cond_(&lock_) {}

	~tbox(void)
	{
		for (typename std::list<T*>::iterator it = tbox_.begin();
			it != tbox_.end(); ++it)
		{
			delete *it;
		}
	}

	/**
	 * 发送消息对象
	 * @param t {T*} 非空消息对象
	 */
	void push(T* t)
	{
		lock_.lock();
		tbox_.push_back(t);
		size_++;
		lock_.unlock();
		cond_.notify();
	}

	/**
	 * 接收消息对象
	 * @param wait_ms {int} >= 0 时设置等待超时时间(毫秒级别)，
	 *  否则永远等待直到读到消息对象或出错
	 * @return {T*} 非 NULL 表示获得一个消息对象
	 */
	T* pop(int wait_ms = -1)
	{
		long long n = ((long long) wait_ms) * 1000;
		lock_.lock();
		while (true)
		{
			T* t = peek();
			if (t)
			{
				lock_.unlock();
				return t;
			}

			if (!cond_.wait(n, true) && wait_ms >= 0)
			{
				lock_.unlock();
				return NULL;
			}
		}
	}

	/**
	 * 返回当前存在于消息队列中的消息数量
	 * @return {size_t}
	 */
	size_t size(void) const
	{
		size_t n;
		lock_.lock();
		n = size_;
		lock_.unlock();
		return n;
	}

private:
	std::list<T*>     tbox_;
	size_t            size_;
	thread_mutex lock_;
	thread_cond  cond_;

	T* peek(void)
	{
		typename std::list<T*>::iterator it = tbox_.begin();
		if (it == tbox_.end())
			return NULL;
		size_--;
		T* t = *it;
		tbox_.erase(it);
		return t;
	}
};

} // namespace acl
