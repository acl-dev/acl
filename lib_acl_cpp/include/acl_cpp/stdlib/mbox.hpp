#pragma once
#include "../acl_cpp_define.hpp"
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

template<typename T>
class mbox
{
public:
	mbox(void)
	{
		mbox_ = mbox_create();
	}

	~mbox(void)
	{
		mbox_free(mbox_, mbox_free_fn);
	}

	/**
	 * 发送消息对象
	 * @param t {T*} 非空消息对象
	 * @return {bool} 发送是否成功
	 */
	bool push(T* t)
	{
		return mbox_send(mbox_, t);
	}

	/**
	 * 接收消息对象
	 * @param timeout {int} 大于 0 时设置读等待超时时间(毫秒级别)，否则
	 *  永远等待直到读到消息对象或出错
	 * @param success {bool*} 可以用于辅助确定读操作是否成功
	 * @return {T*} 非 NULL 表示读到一个消息对象，为 NULL 时，还需通过
	 *  success 参数的返回值检查操作是否成功
	 */
	T* pop(int timeout = 0, bool* success = NULL)
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

	static void mbox_free_fn(void* o)
	{
		T* t = (T*) o;
		delete t;
	}
};

} // namespace acl
