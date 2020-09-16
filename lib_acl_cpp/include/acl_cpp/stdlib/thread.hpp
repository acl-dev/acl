#pragma once
#include "../acl_cpp_define.hpp"
#include "noncopyable.hpp"

namespace acl
{

/**
 * 纯虚函数：线程任务类，该类实例的 run 方法是在子线程中被执行的
 */
class ACL_CPP_API thread_job : public noncopyable
{
public:
	thread_job(void) {}
	virtual ~thread_job(void) {}

	/**
	 * 纯虚函数，子类必须实现此函数，该函数在子线程中执行
	 * @return {void*} 线程退出前返回的参数
	 */
	virtual void* run(void) = 0;

	/**
	 * 虚方法，在新创建的子线程中的 run() 方法被调用前调用，在同步创建
	 * 线程方式下，子线程被创建后调用该虚方法，然后再通知创建这线程，
	 * 从而保证在创建线程的 start() 方法返回前子线程执行初始化过程。
	 */
	virtual void init(void) {}
};

template<typename T> class tbox;
class atomic_long;

/**
 * 线程纯虚类，该类的接口定义类似于 Java 的接口定义，子类需要实现
 * 基类的纯虚函数，使用者通过调用 thread::start() 启动线程过程
 */
class ACL_CPP_API thread : public thread_job
{
public:
	thread(void);
	virtual ~thread(void);

	/**
	 * 开始启动线程过程，一旦该函数被调用，则会立即启动一个新的
	 * 子线程，在子线程中执行基类 thread_job::run 过程
	 * @return {bool} 是否成功创建线程
	 */
	bool start(bool sync = false);

	/**
	 * 当创建线程时为非 detachable 状态，则必须调用此函数等待线程结束；
	 * 若创建线程时为 detachable 状态时，禁止调用本函数
	 * @param out {void**} 当该参数非空指针时，该参数用来存放
	 *  线程退出前返回的参数
	 * @return {bool} 是否成功
	 */
	bool wait(void** out = NULL);

	/**
	 * 在调用 start 前调用此函数可以设置所创建线程是否为分离 (detachable)
	 * 状态；如果未调用此函数，则所创建的线程默认为非分离状态，在非分离状
	 * 态下，其它线程可以 wait 本线程对象，否则禁止 wait 本线程对象；在非
	 * 分离状态下，其它线程必须要 wait 本线程，否则会引起内存泄露。
	 * @param yes {bool} 是否为分离状态
	 * @return {thread&}
	 */
	thread& set_detachable(bool yes);

	/**
	 * 在调用 start 前调用此函数可以设置所创建线程的堆栈大小
	 * @param size {size_t} 线程堆栈大小，当该值为 0 或未
	 *  调用此函数，则所创建的线程堆栈大小为系统的默认值
	 * @return {thread&}
	 */
	thread& set_stacksize(size_t size);

	/**
	 * 在调用 start 后调用此函数可以获得所创建线程的 id 号
	 * @return {unsigned long}
	 */
	unsigned long thread_id(void) const;

	/**
	 * 当前调用者所在线程的线程 id 号
	 * @return {unsigned long}
	 */
	static unsigned long thread_self(void);
	static unsigned long self(void)
	{
		return thread_self();
	}

private:
	bool detachable_;
	size_t stack_size_;
#if defined(_WIN32) || defined(_WIN64)
	void* thread_;
	unsigned long thread_id_;
#else
	pthread_t thread_;
	unsigned long thread_id_;
#endif
	tbox<int>*   sync_;
	atomic_long* lock_;

	void* return_arg_;
	static void* thread_run(void* arg);

	void wait_for_running(void);
};

} // namespace acl
