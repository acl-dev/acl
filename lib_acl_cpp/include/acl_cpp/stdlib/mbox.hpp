#pragma once
#include "../acl_cpp_define.hpp"
#include <assert.h>
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
 *     myobj(void) {}
 *     ~myobj(void) {}
 *     
 *     void run(void)
 *     {
 *         printf("hello world!\r\n");
 *     }
 * };
 * 
 * acl::mbox<myobj> mbox;
 *
 * void thread_producer(void)
 * {
 *     myobj* o = new myobj;
 *     mbox.push(o);
 * }
 * 
 * void thread_consumer(void)
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
	mbox(bool free_obj = true, bool mpsc = true)
	: free_obj_(free_obj)
	{
		mbox_ = mbox_create(mpsc);
		assert(mbox_);
	}

	~mbox(void)
	{
		mbox_free(mbox_, free_obj_ ? mbox_free_fn : NULL);
	}

	/**
	 * 发送消息对象
	 * @param t {T*} 非空消息对象
	 * @param dummy {bool} 目前无任何用处，仅是为了与 tbox 接口一致
	 * @return {bool} 发送是否成功
	 * @override
	 */
	bool push(T* t, bool dummy = false)
	{
		(void) dummy;
		return mbox_send(mbox_, t);
	}

	/**
	 * 接收消息对象
	 * @param timeout {int} >= 0 时设置读等待超时时间(毫秒级别)，否则
	 *  永远等待直到读到消息对象或出错
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
	T* pop(int timeout = -1, bool* found = NULL)
	{
		return (T*) mbox_read(mbox_, timeout, found);
	}

	/**
	 * 统计当前已经发送的消息数
	 * @return {size_t}
	 */
	size_t push_count(void) const
	{
		return mbox_nsend(mbox_);
	}

	/**
	 * 统计当前已经接收到的消息数
	 * @return {size_t}
	 */
	size_t pop_count(void) const
	{
		return mbox_nread(mbox_);
	}

private:
	void* mbox_;
	bool  free_obj_;

	static void mbox_free_fn(void* o)
	{
		T* t = (T*) o;
		delete t;
	}
};

} // namespace acl
