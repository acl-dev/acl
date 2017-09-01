#pragma once
#include "../acl_cpp_define.hpp"
#include "noncopyable.hpp"

struct ACL_MBOX;

namespace acl
{

//class ACL_CPP_API mbox : public noncopyable
template<typename T>
class mbox
{
public:
	mbox(void);
	~mbox(void) {}

	/**
	 * 发送消息对象
	 * @param t {T*} 非空消息对象
	 * @return {bool} 发送是否成功
	 */
	bool push(T* t)
	{
		return push_one((void*) t);
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
		return (T*) pop_one(timeout, success);
	}

	/**
	 * 统计当前已经发送的消息数
	 * @return {size_t}
	 */
	size_t push_count(void) const;

	/**
	 * 统计当前已经接收到的消息数
	 * @return {size_t}
	 */
	size_t pop_count(void) const;

private:
	ACL_MBOX* mbox_;

	bool  push_one(void* o);
	void* pop_one(int timeout, bool* success);
};

} // namespace acl
