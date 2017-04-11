#pragma once
#include <stddef.h>

struct ACL_FIBER;

namespace acl {

/**
 * 协程类定义，纯虚类，需要子类继承并实现纯虚方法
 */
class fiber
{
public:
	/**
	 * 构造函数
	 * @param running {bool} 当为 true 时，则表示当前协程已启动，仅是声明
	 *  了一个协程对象而已，以便于与 ACL_FIBER 对象绑定，此时禁止调用本对
	 *  象的 start 方法启动新协程; 当为 false 时，则需要调用 start 方法来
	 *  启动新协程
	 */
	fiber(bool running = false);
	virtual ~fiber(void);

	/**
	 * 在创建一个协程类对象且构造参数 running 为 false 时，需要本函数启动
	 * 协程，然后子类的重载的 run 方法将被回调，如果 running 为 true 时，
	 * 则禁止调用 start 方法
	 * @param stack_size {size_t} 创建的协程对象的栈大小
	 */
	void start(size_t stack_size = 320000);

	/**
	 * 在本协程运行时调用此函数通知该协程退出
	 * @return {bool} 返回 false 表示本协程未启动或已经退出
	 */
	bool kill(void);

	/**
	 * 判断当前协程是否被通知退出
	 * @return {bool} 本协程是否被通知退出
	 */
	bool killed(void) const;

	/**
	 * 判断当前正在运行的协程是否被通知退出，该方法与 killed 的区别为，
	 * killed 首先必须有 acl::fiber 对象依托，且该协程对象有可能正在运行，
	 * 也有可能被挂起，而 self_killed 不需要 acl::fiber 对象依托且一定表示
	 * 当前正在运行的协程
	 * @return {bool}
	 */
	static bool self_killed(void);

	/**
	 * 获得本协程对象的 ID 号
	 * @return {unsigned int}
	 */
	unsigned int get_id(void) const;

	/**
	 * 获得当前运行的协程对象的 ID 号
	 * @return {unsigned int}
	 */
	static unsigned int self(void);

	/**
	 * 获得当前协程在执行某个系统 API 出错时的错误号
	 * return {int}
	 */
	int get_errno(void) const;

	/**
	 * 设置当前协程的错误号
	 * @param errnum {int}
	 */
	void set_errno(int errnum);

	/**
	 * 启动协程运行的调度过程
	 */
	static void schedule(void);

	/**
	 * 判断当前线程是否处于协程调度状态
	 * @return {bool}
	 */
	static bool scheduled(void);

	/**
	 *  停止协程调度过程
	 */
	static void schedule_stop(void);

public:
	/**
	 * 将当前正在运行的协程(即本协程) 挂起
	 */
	static void yield(void);

	/**
	 * 挂起当前协程，执行等待队列中的下一个协程
	 */
	static void switch_to_next(void);

	/**
	 * 将指定协程对象置入待运行队列中
	 * @param f {fiber&}
	 */
	static void ready(fiber& f);

	/**
	 * 线程启动后调用此函数设置当前线程是否需要 hook 系统 API，内部缺省
	 * 会 hook 系统 API
	 * @param on {bool}
	 */
	static void hook_api(bool on);

public:
	/**
	 * 返回本协程对象对应的 C 语言的协程对象
	 * @return {ACL_FIBER *}
	 */
	ACL_FIBER *get_fiber(void) const;

protected:
	/**
	 * 虚函数，子类须实现本函数，当通过调用 start 方法启动协程后，本
	 * 虚函数将会被调用，从而通知子类协程已启动; 如果在构造函数中的参数
	 * running 为 true ，则 start 将被禁止调用，故本虚方法也不会被调用
	 */
	virtual void run(void);

private:
	ACL_FIBER *f_;

	static void fiber_callback(ACL_FIBER *f, void *ctx);
};

} // namespace acl

#if defined(__GNUC__) && (__GNUC__ > 6 ||(__GNUC__ == 6 && __GNUC_MINOR__ >= 0))
# ifndef   ACL_USE_CPP11
#  define  ACL_USE_CPP11
# endif
#endif

#ifdef	ACL_USE_CPP11

#include <functional>

namespace acl
{

class go_fiber
{
public:
	go_fiber(void) {}
	go_fiber(size_t stack_size) : stack_size_(stack_size) {}

	void operator=(std::function<void()> fn);

private:
	size_t stack_size_ = 320000;
};

} // namespace acl

#define	go		acl::go_fiber()=
#define	go_stack(size)	acl::go_fiber(size)=

/**
 * static void fiber1(void)
 * {
 * 	printf("fiber: %d\r\n", acl::fiber::self());
 * }
 *
 * static void fiber2(acl::string& buf)
 * {
 * 	printf("in fiber: %d, buf: %s\r\n", acl::fiber::self(), buf.c_str());
 * 	buf = "world";
 * }
 *
 * static void fiber3(const acl::string& buf)
 * {
 * 	printf("in fiber: %d, buf: %s\r\n", acl::fiber::self(), buf.c_str());
 * }
 *
 * static test(void)
 * {
 * 	go fiber1;
 * 	
 * 	acl::string buf("hello");
 *
 * 	go[&] {
 * 		fiber2(buf);
 * 	};
 * 	
 * 	go[=] {
 * 		fiber3(buf);
 * 	};
 * 
 * 	go[&] {
 * 		fiber3(buf);
 * 	};
 * }
 */
#endif
