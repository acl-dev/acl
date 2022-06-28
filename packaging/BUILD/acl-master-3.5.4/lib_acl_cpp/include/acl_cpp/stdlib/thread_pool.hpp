#pragma once
#include "noncopyable.hpp"

struct acl_pthread_pool_t;
struct acl_pthread_pool_attr_t;

namespace acl
{

class thread_job;

/**
 * 线程池管理类，该类内管理的线程池中的线程是半驻留的(即当线程空闲一定时间后
 * 自动退出)，该类有两个非纯虚函数：thread_on_init(线程池中的某个线程第一次
 * 创建时会首先调用此函数)，thread_on_exit(线程池中的某个线程退出时调用此函数)
 */
class ACL_CPP_API thread_pool : public noncopyable
{
public:
	thread_pool(void);
	virtual ~thread_pool(void);

	/**
	 * 启动线程池，在创建线程池对象后，必须首先调用此函数以启动线程池
	 */
	void start(void);

	/**
	 * 停止并销毁线程池，并释放线程池资源，调用此函数可以使所有子线程退出，
	 * 但并不释放本实例，如果该类实例是动态分配的则用户应该自释放类实例，
	 * 在调用本函数后，如果想重启线程池过程，则必须重新调用 start 过程
	 */
	void stop(void);

	/**
	 * 等待线程池中的所有线程池执行完所有任务
	 */
	void wait(void);

	/**
	 * 将一个任务交给线程池中的一个线程去执行，线程池中的
	 * 线程会执行该任务中的 run 函数
	 * @param job {thread_job*} 线程任务
	 * @return {bool} 是否成功
	 */
	bool run(thread_job* job);

	/**
	 * 将一个任务交给线程池中的一个线程去执行，线程池中的
	 * 线程会执行该任务中的 run 函数；该函数功能与 run 功能完全相同，只是为了
	 * 使 JAVA 程序员看起来更为熟悉才提供了此接口
	 * @param job {thread_job*} 线程任务
	 * @return {bool} 是否成功
	 */
	bool execute(thread_job* job);

	/**
	 * 在调用 start 前调用此函数可以设置所创建线程的堆栈大小
	 * @param size {size_t} 线程堆栈大小，当该值为 0 或未
	 *  调用此函数，则所创建的线程堆栈大小为系统的默认值
	 * @return {thread&}
	 */
	thread_pool& set_stacksize(size_t size);

	/**
	 * 设置线程池最大线程个数限制
	 * @param max {size_t} 最大线程数，如果不调用此函数，则内部缺省值为 100
	 * @return {thread_pool&}
	 */
	thread_pool& set_limit(size_t max);

	/**
	 * 获得当前线程池最大线程数量限制
	 * @return {size_t}
	 */
	size_t get_limit(void) const;

	/**
	 * 设置线程池中空闲线程的超时退出时间
	 * @param ttl {int} 空闲超时时间(秒)，如果不调用此函数，则内部缺省为 0
	 * @return {thread_pool&}
	 */
	thread_pool& set_idle(int ttl);

	/**
	 * 获得当前线程池中子线程的数量
	 * @return {int} 返回线程池中子线程的数量，如果未通过调用 start
	 *  启动线程池过程，则该函数返回 -1
	 */
	int threads_count(void) const;

	/**
	 * 获得当前线程池中未被处理的任务数量
	 * @return {int} 当线程池还未被启动(即未调用 start)或已经销毁则返回 -1
	 */
	int task_qlen(void) const;

protected:
	/**
	 * 当线程池中的子线程第一次被创建时，该虚函数将被调用，
	 * 用户可以在自己的实现中做一些初始化工作
	 * @return {bool} 初始化是否成功
	 */
	virtual bool thread_on_init(void) { return true; }

	/**
	 * 当线程池中的子线程退出时，该虚函数将被调用，用户可以
	 * 在自己的实现 中做一些资源释放工作
	 */
	virtual void thread_on_exit(void) {}

private:
	size_t stack_size_;
	size_t threads_limit_;
	int    thread_idle_;

	acl_pthread_pool_t* thr_pool_;
	acl_pthread_pool_attr_t* thr_attr_;

	static void thread_run(void* arg);
	static int  thread_init(void* arg);
	static void thread_exit(void* arg);
};

} // namespace acl
