#pragma once
#include "../acl_cpp_define.hpp"
#include "noncopyable.hpp"

// just forward declare functions which are in lib_acl.a

struct ACL_MBOX;

extern "C" {
extern ACL_MBOX *acl_mbox_create(void);
extern void   acl_mbox_free(ACL_MBOX *mbox, void (*free_fn)(void*));
extern int    acl_mbox_send(ACL_MBOX *mbox, void *msg);
extern void  *acl_mbox_read(ACL_MBOX *mbox, int timeout, int *success);
extern size_t acl_mbox_nsend(ACL_MBOX *mbox);
extern size_t acl_mbox_nread(ACL_MBOX *mbox);
}

namespace acl
{

template<typename T>
class mbox
{
public:
	mbox(void)
	{
		mbox_ = acl_mbox_create();
	}

	~mbox(void)
	{
		acl_mbox_free(mbox_, mbox_free_fn);
	}

	/**
	 * 发送消息对象
	 * @param t {T*} 非空消息对象
	 * @return {bool} 发送是否成功
	 */
	bool push(T* t)
	{
		return acl_mbox_send(mbox_, t) == 0;
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
		int ok;
		void* o = (void*) acl_mbox_read(mbox_, timeout, &ok);
		if (success)
			*success = ok ? true : false;
		return (T*) o;
	}

	/**
	 * 统计当前已经发送的消息数
	 * @return {size_t}
	 */
	size_t push_count(void) const
	{
		return acl_mbox_nsend(mbox_);
	}

	/**
	 * 统计当前已经接收到的消息数
	 * @return {size_t}
	 */
	size_t pop_count(void) const
	{
		return acl_mbox_nread(mbox_);
	}

private:
	ACL_MBOX* mbox_;

	static void mbox_free_fn(void* o)
	{
		T* t = (T*) o;
		delete t;
	}
};

} // namespace acl
