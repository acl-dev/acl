#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

namespace acl
{

/**
 * 需要被延迟释放的类继承此类后，可以调用 aio_handle:delay_free 来到达
 * 延迟销毁的目的，避免了在递归过程中被立即释放时的对象被提前释放的问题
 */
class ACL_CPP_API aio_delay_free : public noncopyable
{
public:
	aio_delay_free(void);
	virtual ~aio_delay_free(void);

	/**
	 * 判定定时器是否正处于锁定状态，处于锁定状态的定时器是
	 * 不能被删除的，否则会造成内存严重错误
	 * @return {bool} 是否处于锁定状态，处于锁定状态的对象是
	 *  不允许被销毁的
	 */
	bool locked(void) const;

	/**
	 * 允许子类设置子类对象的锁定对象，这样在定时器处理过程中就不会
	 * 自动调用子类对象的销毁过程
	 */
	void set_locked(void);

	/**
	 * 允许子类取消类对象的锁定状态
	 */
	void unset_locked(void);

	/**
	 * 销毁函数，在内部类 aio_timer_delay_free 对象中对需要做延迟释放
	 * 的类进行销毁
	 */
	virtual void destroy(void) {}

private:
	bool locked_;
	bool locked_fixed_;
};

} // namespace acl
