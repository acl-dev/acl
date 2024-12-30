#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include <stdlib.h>
#include "thread_mutex.hpp"
#include "thread_cond.hpp"
#include "box.hpp"

namespace acl {

/**
 * 用于线程之间的消息通信，通过线程条件变量及线程锁实现
 *
 * 示例：
 *
 * class myobj {
 * public:
 *     myobj(void) {}
 *     ~myobj(void) {}
 *
 *     void test(void) { printf("hello world\r\n"); }
 * };
 *
 * acl::tbox2<myobj> tbox;
 *
 * void thread_producer() {
 *     myobj o;
 *     tbox.push(o);
 * }
 *
 * void thread_consumer() {
 *     myobj o;
 *     if (tbox.pop(o)) {
 *         o->test();
 *     }
 * }
 */

template<typename T>
class tbox2  : public box2<T> {
public:
	/**
	 * 构造方法
	 */
	tbox2() : size_(0), cond_(&lock_) {}

	~tbox2() {}

	/**
	 * 清理消息队列中未被消费的消息对象
	 */
	void clear() {
		box_.clear();
	}

	/**
	 * 发送消息对象
	 * @param t {T} 消息对象
	 * @param notify_first {bool} 如果为 true，则先通知后解锁，否则先解锁
	 *  后通知，注意二者的区别
	 * @return {bool}
	 * @override
	 */
	bool push(T t, bool notify_first = true) {
		if (! lock_.lock()) { abort(); }

#if __cplusplus >= 201103L || defined(USE_CPP11)     // Support c++11 ?
		box_.emplace_back(t);
#else
		box_.push_back(t);
#endif
		size_++;

		if (notify_first) {
			if (!cond_.notify()) { abort(); }
			if (!lock_.unlock()) { abort(); }
		} else {
			if (!lock_.unlock()) { abort(); }
			if (!cond_.notify()) { abort(); }
		}

		return true;
	}

	/**
	 * 接收消息对象
	 * @param t {T&} 当函数 返回 true 时存放结果对象
	 * @param ms {int} >= 0 时设置等待超时时间(毫秒级别)，
	 *  否则永远等待直到读到消息对象或出错
	 * @return {bool} 是否获得消息对象
	 * @override
	 */
	bool pop(T& t, int ms = -1) {
		long long us = ((long long) ms) * 1000;
		if (!lock_.lock()) { abort(); }
		while (true) {
			if (peek(t)) {
				if (!lock_.unlock()) { abort(); }
				return true;
			}

			// 注意调用顺序，必须先调用 wait 再判断 wait_ms
			if (!cond_.wait(us, true) && us >= 0) {
				if (!lock_.unlock()) { abort(); }
				return false;
			}
		}
	}

	// @override
	size_t pop(std::vector<T>& out, size_t max, int ms) {
		long long us = ((long long) ms) * 1000;
		size_t n = 0;
		if (!lock_.lock()) { abort(); }
		while (true) {
			T t;
			if (peek(t)) {
				out.push_back(t);
				n++;
				if (max > 0 && n >= max) {
					if (!lock_.unlock()) { abort(); }
				}
				continue;
			}
			if (n > 0) {
				if (!lock_.unlock()) { abort(); }
				return n;
			}
			if (!cond_.wait(us, true) && us >= 0) {
				if (!lock_.unlock()) { abort(); }
				return n;
			}
		}
	}

	/**
	 * 返回当前存在于消息队列中的消息数量
	 * @return {size_t}
	 * @override
	 */
	size_t size() const {
		return size_;
	}

	// @override
	bool has_null() const {
		return true;
	}

public:
	void lock() {
		if (!lock_.lock()) { abort(); }
	}

	void unlock() {
		if (!lock_.unlock()) { abort(); }
	}

private:
	tbox2(const tbox2&) {}
	const tbox2& operator=(const tbox2&);
private:
	std::list<T> box_;
	size_t       size_;
	thread_mutex lock_;
	thread_cond  cond_;

	bool peek(T& t) {
		typename std::list<T>::iterator it = box_.begin();
		if (it == box_.end()) {
			return false;
		}
#if __cplusplus >= 201103L || defined(USE_CPP11)     // Support c++11 ?
		t = std::move(*it);
#else
		t = *it;
#endif
		box_.erase(it);
		size_--;
		return true;
	}
};

} // namespace acl
