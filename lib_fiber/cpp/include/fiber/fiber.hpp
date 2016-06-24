#pragma once

struct FIBER;

namespace acl {

/**
 * 协程类定义，纯虚类，需要子类继承并实现纯虚方法
 */
class fiber
{
public:
	fiber(void);
	virtual ~fiber(void);

	/**
	 * 在创建一个协程类后，需要本函数启动协程
	 * @param stack_size {size_t} 创建的协程对象的栈大小
	 */
	void start(size_t stack_size = 64000);

	/**
	 * 获得本协程对象的 ID 号
	 * @return {int}
	 */
	int get_id(void) const;

	/**
	 * 获得当前运行的协程对象的 ID 号
	 */
	static int self(void);

	/**
	 * 启动协程运行的调度过程
	 */
	static void schedule();

protected:
	/**
	 * 纯虚函数，子类必须实现本函数，当通过调用 start 方法启动协程后，本
	 * 虚函数将会被调用，从而通知子类
	 */
	virtual void run(void) = 0;

private:
	FIBER *f_;

	static void fiber_callback(FIBER *f, void *ctx);
};

} // namespace acl
