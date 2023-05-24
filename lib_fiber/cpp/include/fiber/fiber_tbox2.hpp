#pragma once
#include "fiber_cpp_define.hpp"
#include <list>
#include <stdlib.h>
#include "fiber_mutex.hpp"
#include "fiber_cond.hpp"

namespace acl {

/**
 * 用于协程之间，线程之间以及协程与线程之间的消息通信，通过协程条件变量
 * 及协程事件锁实现
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
 * acl::fiber_tbox2<myobj> tbox;
 *
 * void thread_producer(void) {
 *     myobj o;
 *     tbox.push(o);
 * }
 *
 * void thread_consumer(void) {
 *     myobj o;

 *     if (tbox.pop(o)) {
 *         o.test();
 *     }
 * }
 */

// The fiber_tbox2 has an object copying process in push/pop which is suitable
// for transfering the object managed by std::shared_ptr.

template<typename T>
class fiber_tbox2 {
public:
	/**
	 * 构造方法
	 */
	fiber_tbox2(void) : size_(0) {}

	~fiber_tbox2(void) {}

	/**
	 * 清理消息队列中未被消费的消息对象
	 */
	void clear(void) {
		tbox_.clear();
	}

	/**
	 * 发送消息对象
	 * @param t {T} 消息对象
	 * @param notify_first {bool} 如果本参数为 true，则内部添加完消息后
	 *  采用先通知后解锁方式，否则采用先解锁后通知方式，当 fiber_tbox2 对象
	 *  的生存周期比较长时，该参数设为 false 的效率更高，如果 fiber_tbox2
	 *  对象的生存周期较短(如：等待者调用 pop 后直接销毁 fiber_tbox2 对象),
	 *  则本参数应该设为 true，以避免 push 者还没有完全返回前因 fiber_tbox2
	 *  对象被提前销毁而造成内存非法访问
	 * @return {bool}
	 */
	bool push(T t, bool notify_first = true) {
		// 先加锁
		if (mutex_.lock() == false) {
			abort();
		}

		// 向队列中添加消息对象
		tbox_.push_back(t);
		size_++;

		if (notify_first) {
			if (cond_.notify() == false) {
				abort();
			}
			if (mutex_.unlock() == false) {
				abort();
			}
			return true;
		} else {
			if (mutex_.unlock() == false) {
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
	 * @param t {T&} 当函数 返回 true 时存放结果对象
	 * @param wait_ms {int} >= 0 时设置等待超时时间(毫秒级别)，
	 *  否则永远等待直到读到消息对象或出错
	 * @return {bool} 是否获得消息对象
	 */
	bool pop(T& t, int wait_ms = -1) {
		if (mutex_.lock() == false) {
			abort();
		}
		while (true) {
			if (peek_obj(t)) {
				if (mutex_.unlock() == false) {
					abort();
				}
				return true;
			}

			if (!cond_.wait(mutex_, wait_ms) && wait_ms >= 0) {
				if (mutex_.unlock() == false) {
					abort();
				}
				return false;
			}
		}
	}

	/**
	 * 返回当前存在于消息队列中的消息数量
	 * @return {size_t}
	 */
	size_t size(void) const {
		return size_;
	}

public:
	void lock(void) {
		if (mutex_.lock() == false) {
			abort();
		}
	}

	void unlock(void) {
		if (mutex_.unlock() == false) {
			abort();
		}
	}

private:
	fiber_tbox2(const fiber_tbox2&) {}
	const fiber_tbox2& operator=(const fiber_tbox2&);

private:
	std::list<T>  tbox_;
	size_t        size_;
	fiber_mutex   mutex_;
	fiber_cond    cond_;

	bool peek_obj(T& t) {
		typename std::list<T>::iterator it = tbox_.begin();
		if (it == tbox_.end()) {
			return false;
		}
		size_--;
		t = *it;
		tbox_.erase(it);
		return true;
	}
};

} // namespace acl
