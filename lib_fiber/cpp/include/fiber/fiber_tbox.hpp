#pragma once
#include "fiber_cpp_define.hpp"
#include <list>
#include <stdlib.h>
#include "fiber_event.hpp"
#include "fiber_cond.hpp"

#if !defined(_WIN32) && !defined(_WIN64)

namespace acl {

/**
 * 用于协程之间，线程之间以及协程与线程之间的消息通信，通过协程条件变量
 * 及协程事件锁实现
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
 * acl::fiber_tbox<myobj> fiber_tbox;
 *
 * void thread_producer(void)
 * {
 *     myobj* o = new myobj;
 *     fiber_tbox.push(o);
 * }
 *
 * void thread_consumer(void)
 * {
 *     myobj* o = fiber_tbox.pop();
 *     o->test();
 *     delete o;
 * }
 */

template<typename T>
class fiber_tbox
{
public:
	/**
	 * 构造方法
	 * @param free_obj {bool} 当 fiber_tbox 销毁时，是否自动检查并释放
	 *  未被消费的动态对象
	 */
	fiber_tbox(bool free_obj = true) : size_(0), free_obj_(free_obj) {}

	~fiber_tbox(void)
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
	 * @param notify_first {bool} 如果本参数为 true，则内部添加完消息后
	 *  采用先通知后解锁方式，否则采用先解锁后通知方式，当 fiber_tbox 对象
	 *  的生存周期比较长时，该参数设为 false 的效率更高，如果 fiber_tbox
	 *  对象的生存周期较短(如：等待者调用 pop 后直接销毁 fiber_tbox 对象),
	 *  则本参数应该设为 true，以避免 push 者还没有完全返回前因 fiber_tbox
	 *  对象被提前销毁而造成内存非法访问
	 * @return {bool}
	 */
	bool push(T* t, bool notify_first = true)
	{
		// 先加锁
		if (event_.wait() == false) {
			abort();
		}

		// 向队列中添加消息对象
		tbox_.push_back(t);
		size_++;

		if (notify_first) {
			if (cond_.notify() == false) {
				abort();
			}
			if (event_.notify() == false) {
				abort();
			}
			return true;
		} else {
			if (event_.notify() == false) {
				abort();
			}
			if (cond_.notify() == false) {
				abort();
			}
			return true;
		}
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
		bool found_flag;
		if (event_.wait() == false) {
			abort();
		}
		while (true) {
			T* t = peek(found_flag);
			if (found_flag) {
				if (event_.notify() == false) {
					abort();
				}
				if (found) {
					*found = found_flag;
				}
				return t;
			}

			// 注意调用顺序，必须先调用 wait 再判断 wait_ms
			if (!cond_.wait(event_, wait_ms) && wait_ms >= 0) {
				if (event_.notify() == false) {
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
		if (event_.wait() == false) {
			abort();
		}
	}

	void unlock(void)
	{
		if (event_.notify() == false) {
			abort();
		}
	}

private:
	fiber_tbox(const fiber_tbox&) {}
	const fiber_tbox& operator=(const fiber_tbox&);

private:
	std::list<T*> tbox_;
	size_t        size_;
	bool          free_obj_;
	fiber_event   event_;
	fiber_cond    cond_;

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

#endif
