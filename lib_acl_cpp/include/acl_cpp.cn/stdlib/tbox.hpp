#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include <cstdlib>
#include "thread_mutex.hpp"
#include "thread_cond.hpp"
#include "noncopyable.hpp"
#include "box.hpp"

namespace acl {

/**
 * 用于线程之间的消息通信，通过线程条件变量及线程锁实现
 *
 * 示例：
 *
 * class myobj {
 * public:
 *     myobj() {}
 *     ~myobj() {}
 *
 *     void test() { printf("hello world\r\n"); }
 * };
 *
 * acl::tbox<myobj> tbox;
 *
 * void thread_producer() {
 *     myobj* o = new myobj;
 *     tbox.push(o);
 * }
 *
 * void thread_consumer() {
 *     myobj* o = tbox.pop();
 *     o->test();
 *     delete o;
 * }
 */

template<typename T>
class tbox : public box<T> {
public:
	/**
	 * 构造方法
	 * @param free_obj {bool} 当 tbox 销毁时，是否自动检查并释放
	 *  未被消费的动态对象
	 */
	explicit tbox(bool free_obj = true)
	: size_(0), free_obj_(free_obj), cond_(&lock_) {}

	~tbox() {
		clear(free_obj_);
	}

	/**
	 * 清理消息队列中未被消费的消息对象
	 * @param free_obj {bool} 释放调用 delete 方法删除消息对象
	 */
	void clear(bool free_obj = false) {
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
	 * @override
	 */
	bool push(T* t, bool notify_first = true) {
		if (! lock_.lock()) { abort(); }
		tbox_.push_back(t);
		size_++;

		if (notify_first) {
			if (! cond_.notify()) { abort(); }
			if (! lock_.unlock()) { abort(); }
		} else {
			if (! lock_.unlock()) { abort(); }
			if (! cond_.notify()) { abort(); }
		}

		return true;
	}

	/**
	 * 接收消息对象
	 * @param ms {int} >= 0 时设置等待超时时间(毫秒级别)，
	 *  否则永远等待直到读到消息对象或出错
	 * @param found {bool*} 非空时用来存放是否获得了一个消息对象，主要用在
	 *  当允许传递空对象时的检查
	 * @return {T*} 非 NULL 表示获得一个消息对象，返回 NULL 时得需要做进一
	 *  步检查，生产者如果 push 了一个空对象（NULL），则消费者也会获得 NULL，
	 *  但此时仍然认为获得了一个消息对象，只不过为空对象；如果 wait_ms 参数
	 *  为 -1 时返回 NULL 依然认为获得了一个空消息对象，如果 wait_ms 大于
	 *  等于 0 时返回 NULL，则应该检查 found 参数的值为 true 还是 false 来
	 *  判断是否获得了一个空消息对象
	 * @override
	 */
	T* pop(int ms = -1, bool* found = NULL) {
		long long us = ((long long) ms) * 1000;
		bool found_flag;
		if (! lock_.lock()) { abort(); }
		while (true) {
			T* t = peek(found_flag);
			if (found_flag) {
				if (! lock_.unlock()) { abort(); }
				if (found) {
					*found = found_flag;
				}
				return t;
			}

			// 注意调用顺序，必须先调用 wait 再判断 wait_ms
			if (! cond_.wait(us, true) && us >= 0) {
				if (! lock_.unlock()) { abort(); }
				if (found) {
					*found = false;
				}
				return NULL;
			}
		}
	}

	// @override
	size_t pop( std::vector<T*>& out, size_t max, int ms) {
		long long us = ((long long) ms) * 1000;
		size_t n = 0;
		bool found_flag;

		if (! lock_.lock()) { abort(); }
		while (true) {
			T* t = peek(found_flag);
			if (found_flag) {
				out.push_back(t);
				n++;
				if (max > 0 && n >= max) {
					return n;
				}
				continue;
			}

			if (n > 0) {
				if (! lock_.unlock()) { abort(); }
				return n;
			}

			if (! cond_.wait(us, true) && us >= 0) {
				if (! lock_.unlock()) { abort(); }
				return n;
			}
		}
	}

	/**
	 * tbox 支持传递空消息
	 * @return {bool}
	 * @override
	 */
	bool has_null() const {
		return true;
	}

	/**
	 * 返回当前存在于消息队列中的消息数量
	 * @return {size_t}
	 */
	size_t size() const {
		return size_;
	}

public:
	void lock() {
		if (! lock_.lock()) { abort(); }
	}

	void unlock() {
		if (! lock_.unlock()) { abort(); }
	}

private:
	std::list<T*> tbox_;
	size_t        size_;
	bool          free_obj_;
	thread_mutex lock_;
	thread_cond  cond_;

	T* peek(bool& found_flag) {
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
