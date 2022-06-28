#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include <stdlib.h>
#include <string.h>
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
 * acl::tbox_array<myobj> tbox;
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
class tbox_array : public noncopyable
{
public:
	/**
	 * 构造方法
	 * @param free_obj {bool} 当 tbox_array 销毁时，是否自动检查并释放
	 *  未被消费的动态对象
	 */
	tbox_array(bool free_obj = true)
	: capacity_(10000)
	, off_curr_(0)
	, off_next_(0)
	, waiters_(0)
	, free_obj_(free_obj)
	, cond_(&lock_)
	{
		array_ = (T**) malloc(sizeof(T*) * capacity_);
	}

	~tbox_array(void)
	{
		clear(free_obj_);
		free(array_);
	}

	/**
	 * 清理消息队列中未被消费的消息对象
	 * @param free_obj {bool} 释放调用 delete 方法删除消息对象
	 */
	void clear(bool free_obj = false)
	{
		if (free_obj) {
			for (size_t i = off_curr_; i < off_next_; i++) {
				delete array_[i];
			}
		}
	}

	/**
	 * 发送消息对象，本过程是添加完对象后先解锁后通知
	 * @param t {T*} 非空消息对象
	 * @param notify_first {bool} 如果为 true，则先通知后解锁，否则先解锁
	 *  后通知，注意二者的区别
	 * @return {bool}
	 */
	bool push(T* t, bool notify_first = false)
	{
		if (lock_.lock() == false) {
			abort();
		}

		if (off_next_ == capacity_) {
			if (off_curr_ >= 10000) {
#if 1
				size_t n = 0;
				for (size_t i = off_curr_; i < off_next_; i++) {
					array_[n++] = array_[i];
				}
#else
				memmove(array_, array_ + off_curr_,
					(off_next_ - off_curr_) * sizeof(T*));
#endif

				off_next_ -= off_curr_;
				off_curr_ = 0;
			} else {
				capacity_ += 10000;
				array_ = (T**) realloc(array_, sizeof(T*) * capacity_);
			}
		}
		array_[off_next_++] = t;

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
			waiters_++;
			if (!cond_.wait(n, true) && wait_ms >= 0) {
				waiters_--;
				if (lock_.unlock() == false) {
					abort();
				}
				if (found) {
					*found = false;
				}
				return NULL;
			}
			waiters_--;
		}
	}

	/**
	 * 返回当前存在于消息队列中的消息数量
	 * @return {size_t}
	 */
	size_t size(void) const
	{
		return off_next_ - off_curr_;
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
	T**          array_;
	size_t       capacity_;
	size_t       off_curr_;
	size_t       off_next_;
	size_t       waiters_;
	bool         free_obj_;
	thread_mutex lock_;
	thread_cond  cond_;

	T* peek(bool& found_flag)
	{
		if (off_curr_ == off_next_) {
			found_flag = false;
			if (off_curr_ > 0) {
				off_curr_ = off_next_ = 0;
			}
			return NULL;
		}

		found_flag = true;
		T* t = array_[off_curr_++];
		return t;
	}
};

} // namespace acl
