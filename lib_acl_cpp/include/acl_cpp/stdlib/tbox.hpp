#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include <stdlib.h>
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
	/**
	 * 构造方法
	 * @param free_obj {bool} 当 tbox 销毁时，是否自动检查并释放
	 *  未被消费的动态对象
	 */
	tbox(bool free_obj = true)
	: size_(0), free_obj_(free_obj), cond_(&lock_) {}

	~tbox(void)
	{
		clear(free_obj_);
	}

	/**
	 * 清理消息队列中未被消费的消息对象
	 * @param free_obj {bool} 释放调用 delete 方法删除消息对象
	 */
	void clear(bool free_obj = false)
	{
		if (free_obj) {
			for (typename std::list<T*>::iterator it =
				tbox_.begin(); it != tbox_.end(); ++it) {

				delete *it;
			}
		}
		tbox_.clear();
	}

	/**
	 * 发送消息对象
	 * @param t {T*} 非空消息对象
	 * @param notify_first {bool} 如果为 true，则先通知后解锁，否则先解锁
	 *  后通知，注意二者的区别
	 * @return {bool}
	 */
	bool push(T* t, bool notify_first = true)
	{
		if (lock_.lock() == false) {
			abort();
		}
		tbox_.push_back(t);
		size_++;

		if (notify_first) {
			if (cond_.notify() == false) {
				abort();
			}
			if (lock_.unlock() == false) {
				abort();
			}
		} else {
			if (lock_.unlock() == false) {
				abort();
			}
			if (cond_.notify() == false) {
				abort();
			}
		}

		return true;
	}

	/**
	 * 接收消息对象
	 * @param wait_ms {int} >= 0 时设置等待超时时间(毫秒级别)，
	 *  否则永远等待直到读到消息对象或出错
	 * @param found {bool*} 非空时用来存放是否获得了一个消息对象，主要用在
	 *  当允许传递空对象时的检查
	 * @return {T*} 非 NULL 表示获得一个消息对象，返回 NULL 时得需要做进一
	 *  步检查，生产者如果 push 了一个空对象（NULL），则消费者也会获得 NULL，
	 *  但此时仍然认为获得了一个消息对象，只不过为空对象；如果 wait_ms 参数
	 *  为 -1 时返回 NULL 依然认为获得了一个空消息对象，如果 wait_ms 大于
	 *  等于 0 时返回 NULL，则应该检查 found 参数的值为 true 还是 false 来
	 *  判断是否获得了一个空消息对象
	 */
	T* pop(int wait_ms = -1, bool* found = NULL)
	{
		long long n = ((long long) wait_ms) * 1000;
		bool found_flag;
		if (lock_.lock() == false) {
			abort();
		}
		while (true) {
			T* t = peek(found_flag);
			if (found_flag) {
				if (lock_.unlock() == false) {
					abort();
				}
				if (found) {
					*found = found_flag;
				}
				return t;
			}

			// 注意调用顺序，必须先调用 wait 再判断 wait_ms
			if (!cond_.wait(n, true) && wait_ms >= 0) {
				if (lock_.unlock() == false) {
					abort();
				}
				if (found) {
					*found = false;
				}
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
		return size_;
	}

public:
	void lock(void)
	{
		if (lock_.lock() == false) {
			abort();
		}
	}

	void unlock(void)
	{
		if (lock_.unlock() == false) {
			abort();
		}
	}

private:
	std::list<T*> tbox_;
	size_t        size_;
	bool          free_obj_;
	thread_mutex lock_;
	thread_cond  cond_;

	T* peek(bool& found_flag)
	{
		typename std::list<T*>::iterator it = tbox_.begin();
		if (it == tbox_.end()) {
			found_flag = false;
			return NULL;
		}
		found_flag = true;
		size_--;
		T* t = *it;
		tbox_.erase(it);
		return t;
	}
};

} // namespace acl
