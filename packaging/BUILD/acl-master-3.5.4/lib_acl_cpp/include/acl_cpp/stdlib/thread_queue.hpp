#pragma once
#include "../acl_cpp_define.hpp"
#include "noncopyable.hpp"

struct ACL_AQUEUE;

namespace acl
{

class ACL_CPP_API thread_qitem : public noncopyable
{
public:
	thread_qitem() {}
	virtual ~thread_qitem() {}
};

class ACL_CPP_API thread_queue : public noncopyable
{
public:
	thread_queue();
	~thread_queue();

	bool push(thread_qitem* item);
	thread_qitem* pop(int wait_ms = -1);
	int qlen() const;

private:
	ACL_AQUEUE* queue_;
};

//////////////////////////////////////////////////////////////////////////////

#if 0

// internal functions being used
void*  tbox_create(void);
void   tbox_free(void*, void (*free_fn)(void*));
bool   tbox_push(void*, void*);
void*  tbox_pop(void*, int);
size_t tbox_size(void*);

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
class tbox : noncopyable
{
public:
	tbox(void)
	{
		tbox_ = tbox_create();
	}

	~tbox(void)
	{
		tbox_free(tbox_, tbox_free_fn);
	}

	/**
	 * 发送消息对象
	 * @param t {T*} 非空消息对象
	 * @return {bool} 发送是否成功
	 */
	bool push(T* t)
	{
		return tbox_push(tbox_, t);
	}

	/**
	 * 接收消息对象
	 * @param wait_ms {int} >= 0 时设置读等待超时时间(毫秒级别)，否则
	 *  永远等待直到读到消息对象或出错
	 * @return {T*} 非 NULL 表示获得一个消息对象
	 */
	T* pop(int wait_ms = -1)
	{
		return (T*) tbox_pop(tbox_, wait_ms);
	}

	/**
	 * 返回当前存在于消息队列中的消息数量
	 * @return {size_t}
	 */
	size_t size(void) const
	{
		return tbox_size(tbox_);
	}

private:
	void* tbox_;

	static void tbox_free_fn(void* o)
	{
		T* t = (T*) o;
		delete t;
	}
};

#endif

} // namespace acl
