#pragma once
#include "../acl_cpp_define.hpp"
#include <cassert>
#include "box.hpp"

namespace acl {

// internal functions being used
void*  mbox_create(bool mpsc);
void   mbox_free(void*, void (*free_fn)(void*));
bool   mbox_send(void*, void*);
void*  mbox_read(void*, int, bool*);
size_t mbox_nsend(void*);
size_t mbox_nread(void*);

/**
 * 可用于在线程之间、协程之间进行通信的类，内部实现采用无锁队列 + IO 通信相
 * 结合方式实现
 *
 * 示例:
 *
 * class myobj {
 * public:
 *     myobj() {}
 *     ~myobj() {}
 *     
 *     void run()
 *     {
 *         printf("hello world!\r\n");
 *     }
 * };
 * 
 * acl::mbox<myobj> mbox;
 *
 * void thread_producer()
 * {
 *     myobj* o = new myobj;
 *     mbox.push(o);
 * }
 * 
 * void thread_consumer()
 * {
 *     myobj* o = mbox.pop();
 *     o->run();
 *     delete o;
 * }
 */

template<typename T>
class mbox : public box<T> {
public:
	/**
	 * 构造方法
	 * @param free_obj {bool} 当 tbox 销毁时，是否自动检查并释放
	 *  未被消费的动态对象
	 * @param mpsc {bool} 是否为多生产者-单消费者模式
	 */
	explicit mbox(bool free_obj = true, bool mpsc = true)
	: free_obj_(free_obj)
	{
		mbox_ = mbox_create(mpsc);
		assert(mbox_);
	}

	~mbox() {
		mbox_free(mbox_, free_obj_ ? mbox_free_fn : NULL);
	}

	/**
	 * 发送消息对象
	 * @param t {T*} 非空消息对象
	 * @return {bool} 发送是否成功
	 * @override
	 */
	bool push(T* t, bool dummy = false) {
        (void) dummy;
		return mbox_send(mbox_, t);
	}

	/**
	 * 接收消息对象
	 * @param ms {int} >= 0 时设置读等待超时时间(毫秒级别)，否则
	 *  永远等待直到读到消息对象或出错
	 * @param success {bool*} 可以用于辅助确定读操作是否成功
	 * @return {T*} 非 NULL 表示读到一个消息对象，为 NULL 时，还需通过
	 *  success 参数的返回值检查操作是否成功
	 * @override
	 */
	T* pop(int ms = -1, bool* success = NULL) {
		return (T*) mbox_read(mbox_, ms, success);
	}

	// @override
	size_t pop(std::vector<T*>& out, size_t max, int ms) {
		size_t n = 0;
		bool success;
		while (true) {
			T* t = (T*) mbox_read(mbox_, ms, &success);
			if (! t) {
				return n;
			}

			out.push_back(t);
			n++;
			if (max > 0 && n >= max) {
				return n;
			}
			ms = 0;
		}
	}

	/**
	 * mbox 不支持传递空消息
	 * @return {bool}
	 * @override
	 */
	bool has_null() const {
		return false;
	}

	/**
	 * 统计当前已经发送的消息数
	 * @return {size_t}
	 */
	size_t push_count() const {
		return mbox_nsend(mbox_);
	}

	/**
	 * 统计当前已经接收到的消息数
	 * @return {size_t}
	 */
	size_t pop_count() const {
		return mbox_nread(mbox_);
	}

private:
	void* mbox_;
	bool  free_obj_;

	static void mbox_free_fn(void* o) {
		T* t = (T*) o;
		delete t;
	}
};

} // namespace acl
