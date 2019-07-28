#ifndef ACL_PTHREAD_POOL_INCLUDE_H
#define ACL_PTHREAD_POOL_INCLUDE_H

#include "../stdlib/acl_define.h"
#include <time.h>

#ifdef	ACL_UNIX
#include <pthread.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct acl_pthread_job_t acl_pthread_job_t;

/**
 * 创建一个线程池的工作任务
 * @param run_fn {void (*)(void*)} 在子线程中被调用的回调函数 
 * @param run_arg {void*} run_fn 的回调参数之一
 * @param fixed {int}
 * @return {acl_pthread_job_t*} 返回创建的工作任务
 */
ACL_API acl_pthread_job_t *acl_pthread_pool_alloc_job(void (*run_fn)(void*),
		void *run_arg, int fixed);

/**
 * 释放由 acl_pthread_pool_alloc_job 创建的工作任务
 * @param job {acl_pthread_job_t*}
 */
ACL_API void acl_pthread_pool_free_job(acl_pthread_job_t *job);

/**
 * 线程池对象结构类型定义
 */
typedef struct acl_pthread_pool_t acl_pthread_pool_t;

/**
 * 线程池对象属性的结构类型定义
 */
typedef struct acl_pthread_pool_attr_t {
	int   threads_limit;	/**< 线程池最大线程数限制 */
#define ACL_PTHREAD_POOL_DEF_THREADS   100  /**< 缺省最大值为 100 个线程 */
	int   idle_timeout;                 /**< 工作线程空闲超时时间(秒) */
#define ACL_PTHREAD_POOL_DEF_IDLE      0    /**< 缺省空间超时时间为 0 秒 */
	size_t stack_size;                  /**< 工作线程的堆栈大小(字节) */
} acl_pthread_pool_attr_t;

/**
 * 更简单地创建线程对象的方法
 * @param threads_limit {int}  线程池中最大并发线程数
 * @param idle_timeout {int} 工作线程空闲超时退出时间(秒)
 * @return {acl_pthread_pool_t*}, 如果不为空则表示成功，否则失败
 */
ACL_API acl_pthread_pool_t *acl_thread_pool_create(
		int threads_limit, int idle_timeout);

/**
 * 创建一个线程池对象
 * @param attr {acl_pthread_pool_attr_t*} 线程池创建时的属性，如果该参数为空，
 *  则采用默认参数: ACL_PTHREAD_POOL_DEF_XXX
 * @return {acl_pthread_pool_t*}, 如果不为空则表示成功，否则失败
 */
ACL_API acl_pthread_pool_t *acl_pthread_pool_create(
		const acl_pthread_pool_attr_t *attr);

/**
 * 当队列堆积的任务数大于空闲线程数的2倍时. 通过此函数设置添加任务的
 * 线程休眠时间, 如果不调用此函数进行设置, 则添加线程不会进入休眠状态.
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @param timewait_sec {int} 休眠　的时间值, 建议将此值设置为 1--5 秒内
 * @return {int} 成功返回 0, 失败返回 -1
 */
ACL_API int acl_pthread_pool_set_timewait(
		acl_pthread_pool_t *thr_pool, int timewait_sec);

/**
 * 添加注册函数，在线程创建后立即执行此初始化函数
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @param init_fn {int (*)(void*)} 工作线程初始化函数, 如果该函数返回 < 0,
 *  则该线程自动退出。
 * @param init_arg {void*} init_fn 所需要的参数
 * @return {int} 0: OK; != 0: Error.
 */
ACL_API int acl_pthread_pool_atinit(acl_pthread_pool_t *thr_pool,
		int (*init_fn)(void *), void *init_arg);

/**
 * 添加注册函数，在线程退出立即执行此初函数
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @param free_fn {void (*)(void*)} 工作线程退出前必须执行的函数
 * @param free_arg {void*} free_fn 所需要的参数
 * @return {int} 0: OK; != 0: Error.
 */
ACL_API int acl_pthread_pool_atfree(acl_pthread_pool_t *thr_pool,
		void (*free_fn)(void *), void *free_arg);

/**
 * 销毁一个线程池对象, 成功销毁后该对象不能再用.
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @return {int} 0: 成功; != 0: 失败
 */
ACL_API int acl_pthread_pool_destroy(acl_pthread_pool_t *thr_pool);

/**
 * 暂停一个线程池对象的运行, 停止后还可以再运行.
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @return {int} 0: 成功; != 0: 失败
 */
ACL_API int acl_pthread_pool_stop(acl_pthread_pool_t *thr_pool);

/**
 * 向线程池添加一个任务
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @param run_fn {void (*)(*)} 当有可用工作线程时所调用的回调处理函数
 * @param run_arg {void*} 回调函数 run_fn 所需要的回调参数
 */
ACL_API void acl_pthread_pool_add_one(acl_pthread_pool_t *thr_pool,
		void (*run_fn)(void *), void *run_arg);
#define	acl_pthread_pool_add	acl_pthread_pool_add_one

/**
 * 向线程池添加一个任务
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @param job {acl_pthread_job_t*} 由 acl_pthread_pool_alloc_job 创建的线程任务
 */
ACL_API void acl_pthread_pool_add_job(acl_pthread_pool_t *thr_pool,
		acl_pthread_job_t *job);

/**
 * 开始进行批处理方式的添加任务, 实际上是开始进行加锁
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 */
ACL_API void acl_pthread_pool_bat_add_begin(acl_pthread_pool_t *thr_pool);

/**
 * 添加一个新任务, 前提是已经成功加锁, 即调用 acl_pthread_pool_bat_add_begin 成功
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @param run_fn {void (*)(void*)} 当有可用工作线程时所调用的回调处理函数
 * @param run_arg 回调函数 run_fn 所需要的回调参数
 */
ACL_API void acl_pthread_pool_bat_add_one(acl_pthread_pool_t *thr_pool,
		void (*run_fn)(void *), void *run_arg);
/**
 * 添加一个新任务, 前提是已经成功加锁, 即调用 acl_pthread_pool_bat_add_begin 成功
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @param job {acl_pthread_job_t*} 由 acl_pthread_pool_alloc_job 创建的线程任务
 */
ACL_API void acl_pthread_pool_bat_add_job(acl_pthread_pool_t *thr_pool,
		acl_pthread_job_t *job);

/**
 * 批处理添加结束, 实际是解锁
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 */
ACL_API void acl_pthread_pool_bat_add_end(acl_pthread_pool_t *thr_pool);

/**
 * 设置线程池 POLLER 调度函数，若要使用此功能，需要在用函数
 * acl_pthread_pool_create 后通过此函数设置调度函数，然后再
 * 调用 acl_pthread_pool_start_poller 启动后台调度函数，该后台
 * 调度函数会不断地调用 poller_fn (即用户的回调函数)，用户可以
 * 在回调函数里调用 acl_pthread_pool_add 将新任务添加进线程池中
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @param poller_fn {int (*)(void*)} 循环检测任务队列的回调函数
 * @param poller_arg {void*} poller_fn 所需要的参数
 */
ACL_API void acl_pthread_pool_set_poller(acl_pthread_pool_t *thr_pool,
		int (*poller_fn)(void *), void *poller_arg);
/**
 * 启动一个线程池 POLLER 调度线程
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @return 0 成功; != 0 失败, 可以对返回值调用 strerror(ret) 取得错误原因描述
 */
ACL_API int acl_pthread_pool_start_poller(acl_pthread_pool_t *thr_pool);

/**
 * 以批处理方式进行任务的分发, 其内部其实是调用了 acl_pthread_pool_add_one()
 * @return 0: OK; -1: err
 */
ACL_API int acl_pthread_pool_add_dispatch(void *dispatch_arg,
		void (*run_fn)(void *), void *run_arg);

/**
 * 以单个添加的方式进行任务的分发
 * @return 0: OK; -1: err
 * 注：worker_fn 中的第二个参数为ACL_WORKER_ATTR结构指针，由线程池的某个
 *     工作线程维护，该结构指针中的成员变量 init_data 为用户的赋值传送变量，
 *     如：数据库连接对象等。
 */
ACL_API int acl_pthread_pool_dispatch(void *dispatch_arg,
		void (*run_fn)(void *), void *run_arg);

/**
 * 获得当前线程池的最大线程数限制
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @return {int} 最大线程数限制值
 */
ACL_API int acl_pthread_pool_limit(acl_pthread_pool_t *thr_pool);

/**
 * 获得当前线程池中的线程数
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @return {int} 返回线程池中的总线程数，返回值 < 0 表示出错
 */
ACL_API int acl_pthread_pool_size(acl_pthread_pool_t *thr_pool);

/**
 * 获得当前线程池中的空闲线程数
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @return {int} 返回线程池中的空闲线程数，返回值 < 0 表示出错
 */
ACL_API int acl_pthread_pool_idle(acl_pthread_pool_t *thr_pool);

/**
 * 获得当前线程池中的繁忙线程数
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @return {int} 返回线程池中的繁忙线程数，返回值 < 0 表示出错
 */
ACL_API int acl_pthread_pool_busy(acl_pthread_pool_t *thr_pool);

/**
 * 设置线程任务调度超时警告的时间(毫秒)
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @param n {acl_int64} 当该值 > 0 时，如果线程任务的调度时间超过此值则会记录警告日志(毫秒)
 */
ACL_API void acl_pthread_pool_set_schedule_warn(
		acl_pthread_pool_t *thr_pool, acl_int64 n);

/**
 * 设置线程池中子线程等待任务的超时基准时间(毫秒)
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @param n {acl_int64} 当该值 > 0 时，子线程等待任务的超时等待基准时间(毫秒)
 */
ACL_API void acl_pthread_pool_set_schedule_wait(
		acl_pthread_pool_t *thr_pool, acl_int64 n);

/**
 * 当线程池中的任务发生堆积时，通过该函数设置任务队列堆积报警值
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @param max {int} 任务队列堆积报警值
 */
ACL_API void acl_pthread_pool_set_qlen_warn(
		acl_pthread_pool_t *thr_pool, int max);
/**
 * 取得当前线程池全局队列中未处理的任务个数
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @return {int} 当前未处理的任务数，返回值 < 0 表示出错
 */
ACL_API int acl_pthread_pool_qlen(acl_pthread_pool_t *thr_pool);

/**
 * 设置线程池中线程的堆栈大小
 * @param thr_pool {acl_pthread_pool_t*} 线程池对象，不能为空
 * @param size {size_t} 线程创建时的堆栈大小，单位为字节
 */
ACL_API void acl_pthread_pool_set_stacksize(
		acl_pthread_pool_t *thr_pool, size_t size);

/**
 * 初始化线程池属性值
 * @param attr {acl_pthread_pool_attr_t*}
 */
ACL_API void acl_pthread_pool_attr_init(acl_pthread_pool_attr_t *attr);

/**
 * 设置线程池属性中的最大堆栈大小(字节)
 * @param attr {acl_pthread_pool_attr_t*}
 * @param size {size_t}
 */
ACL_API void acl_pthread_pool_attr_set_stacksize(
		acl_pthread_pool_attr_t *attr, size_t size);

/**
 * 设置线程池属性中的最大线程数限制值
 * @param attr {acl_pthread_pool_attr_t*}
 * @param threads_limit {int} 线程池中的最大线程数
 */
ACL_API void acl_pthread_pool_attr_set_threads_limit(
		acl_pthread_pool_attr_t *attr, int threads_limit);

/**
 * 设置线程池属性中线程空闲超时值
 * @param attr {acl_pthread_pool_attr_t*}
 * @param idle_timeout {int} 线程空闲超时时间(秒)
 */
ACL_API void acl_pthread_pool_attr_set_idle_timeout(
		acl_pthread_pool_attr_t *attr, int idle_timeout);

#ifdef	__cplusplus
}
#endif

#endif	/* !__acl_pthread_pool_t_H_INCLUDED__ */
