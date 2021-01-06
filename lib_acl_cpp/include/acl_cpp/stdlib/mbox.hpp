#pragma once
#include "../acl_cpp_define.hpp"
#include <assert.h>
#include "noncopyable.hpp"

namespace acl
{

// internal functions being used
void*  mbox_create(void);
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
 * class myobj
 * {
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
class mbox : public noncopyable
{
public:
	/**
	 * 构造方法
	 * @param free_obj {bool} 当 tbox 销毁时，是否自动检查并释放
	 *  未被消费的动态对象
	 */
	mbox(bool free_obj = true)
	: free_obj_(free_obj)
	{
		mbox_ = mbox_create();
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
	 * @param success {bool*} 可以用于辅助确定读操作是否成功
	 * @return {T*} 非 NULL 表示读到一个消息对象，为 NULL 时，还需通过
	 *  success 参数的返回值检查操作是否成功
	 */
	T* pop(int timeout = -1, bool* success = NULL)
	{
		return (T*) mbox_read(mbox_, timeout, success);
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
