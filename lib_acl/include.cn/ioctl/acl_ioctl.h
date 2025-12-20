#ifndef	ACL_IOCTL_INCLUDE_H
#define	ACL_IOCTL_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include "../event/acl_events.h"
#include "../stdlib/acl_vstream.h"

typedef struct ACL_IOCTL ACL_IOCTL;

/**
 * 当数据流可用时的任务回调函数类型定义, 用户需要根据此函数原型实现自己的任务
 * @param event_type {int} 数据流的事件状态, 为下列状态之一:
 *        ACL_EVENT_READ: 数据流有数据可读; 
 *	  ACL_EVENT_WRITE: 数据流有空间可写;
 *	  ACL_EVENT_RW_TIMEOUT: 该数据流读写超时;
 *	  ACL_EVENT_XCPT: 数据流内部出现异常.
 * @param ioc {ACL_IOCTL*} io 控制句柄
 * @param stream {ACL_VSTREAM*} 网络流句柄
 * @param context {void*} 用户自定义对象
 */
typedef void (*ACL_IOCTL_NOTIFY_FN)(int event_type, ACL_IOCTL *ioc,
	ACL_VSTREAM *stream, void *context);

typedef void (*ACL_IOCTL_WORKER_FN)(ACL_IOCTL *ioc, void *arg);
typedef void (*ACL_IOCTL_THREAD_INIT_FN)(void *);
typedef void (*ACL_IOCTL_THREAD_EXIT_FN)(void *);

/*----------------------------------------------------------------------------*/
/* in acl_ioctl.c */
/**
 * 创建一个服务器框架
 * @param max_threads {int} 所创建的任务池内的最大线程数
 * @param idle_timeout {int} 每个线程空闲时间, 当某个纯种的空闲时间超过
 *  此值时会自动退出, 单位为秒
 * @return {ACL_IOCTL*} 服务器任务池句柄
 */
ACL_API ACL_IOCTL *acl_ioctl_create(int max_threads, int idle_timeout);

/**
 * 创建一个服务器框架
 * @param event_mode {int} 事件方式: ACL_EVENT_SELECT/ACL_EVENT_KERNEL
 * @param max_threads {int} 所创建的任务池内的最大线程数
 * @param idle_timeout {int} 每个线程空闲时间, 当某个纯种的空闲时间超过
 *  此值时会自动退出, 单位为秒
 * @param delay_sec {int} 在事件循环中的秒值
 * @param delay_usec {int} 在事件循环中的微秒值
 * @return {ACL_IOCTL*} 服务器任务池句柄
 */
ACL_API ACL_IOCTL *acl_ioctl_create_ex(int event_mode, int max_threads,
	int idle_timeout, int delay_sec, int delay_usec);

/**
 * 为了防止在多线程模式下 select 等事件循环的时间等待，可以添加此项以中断等待，
 * 加快事件循环过程
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 */
ACL_API void acl_ioctl_add_dog(ACL_IOCTL *ioc);

/**
 * 设置服务器任务池的控制参数
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @param name {int} 参数列表中的第一个参数, ACL_IOCTL_CTL_
 * @param ... 变参参数序列
 */
ACL_API void acl_ioctl_ctl(ACL_IOCTL *ioc, int name, ...);
#define	ACL_IOCTL_CTL_END                       0  /**< 控制结束标志 */
#define	ACL_IOCTL_CTL_THREAD_MAX                1  /**< 设置最大线程数 */
#define	ACL_IOCTL_CTL_THREAD_IDLE               2  /**< 设置线程空闲退出时间 */
#define	ACL_IOCTL_CTL_DELAY_SEC                 3  /**< 设置 select 时的秒级休息值 */
#define	ACL_IOCTL_CTL_DELAY_USEC                4  /**< 设置 select 时的微秒级休息值 */
#define	ACL_IOCTL_CTL_INIT_FN                   5  /**< 设置线程被创建时的线程初始化函数 */
#define	ACL_IOCTL_CTL_EXIT_FN                   6  /**< 设置线程退出时的线程退出回调函数 */
#define	ACL_IOCTL_CTL_INIT_CTX                  7  /**< 设置线程初始化时的回调参数 */
#define	ACL_IOCTL_CTL_EXIT_CTX                  8  /**< 设置线程退出时的回调参数 */
#define ACL_IOCTL_CTL_THREAD_STACKSIZE          9  /**< 设置线程的规模尺寸大小(字节) */

/**
 * 销毁任务池资源
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 */
ACL_API void acl_ioctl_free(ACL_IOCTL *ioc);

/**
 * 启动任务工作池
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @return {int} 是否启动服务器任务池正常. 0: ok; < 0: err.
 */
ACL_API int acl_ioctl_start(ACL_IOCTL *ioc);

/**
 * 消息循环(仅适用于单线程模式)
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 */
ACL_API void acl_ioctl_loop(ACL_IOCTL *ioc);

/**
 * 获得事件引擎句柄
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @return {ACL_EVENT*}
 */
ACL_API ACL_EVENT *acl_ioctl_event(ACL_IOCTL *ioc);

/**
 * 将数据流从事件的读、写监听中去除
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @param stream {ACL_VSTREAM*} 客户端数据流指针
 */
ACL_API void acl_ioctl_disable_readwrite(ACL_IOCTL *ioc, ACL_VSTREAM *stream);

/**
 * 将数据流从事件的读监听中去除
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @param stream {ACL_VSTREAM*} 客户端数据流指针
 */
ACL_API void acl_ioctl_disable_read(ACL_IOCTL *ioc, ACL_VSTREAM *stream);

/**
 * 将数据流从事件的写监听中去除
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @param stream {ACL_VSTREAM*} 客户端数据流指针
 */
ACL_API void acl_ioctl_disable_write(ACL_IOCTL *ioc, ACL_VSTREAM *stream);

/**
 * 判断某个流是否处于受监控状态, 只要读或写任何一种状态均返回真
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @param stream {ACL_VSTREAM*} 客户端数据流指针
 * @return {int} 1：表示是; 0: 表示否
 */
ACL_API int acl_ioctl_isset(ACL_IOCTL *ioc, ACL_VSTREAM *stream);

/**
 * 判断某个流是否处理于读监控状态
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @param stream {ACL_VSTREAM*} 客户端数据流指针
 * @return {int} 1：表示是; 0: 表示否
 */
ACL_API int acl_ioctl_isrset(ACL_IOCTL *ioc, ACL_VSTREAM *stream);

/**
 * 判断某个流是否处于写受监控状态
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @param stream {ACL_VSTREAM*} 客户端数据流指针
 * @return {int} 1：表示是; 0: 表示否
 */
ACL_API int acl_ioctl_iswset(ACL_IOCTL *ioc, ACL_VSTREAM *stream);

/**
 * 设置流状, 当流的IO完成时自动关闭流
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @param stream {ACL_VSTREAM*} 客户端数据流指针
 * @return {int} 是否真正被关闭
 *         0: 表示流中还有数据未处理完, 将进入异步关闭过程;
 *         1: 表示流中无未处理数据, 已经被同步关闭
 */
ACL_API int acl_ioctl_iocp_close(ACL_IOCTL *ioc, ACL_VSTREAM *stream);

/**
 * 向任务池中添加一个读监听工作任务
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @param stream {ACL_VSTREAM*} 客户端数据流指针
 * @param timeout {int} 流连接空闲超时时间
 * @param callback {ACL_IOCTL_NOTIFY_FN} 当数据流可读或出错或超时时的回调函数
 * @param context {void*} 回调函数 callback 的参数之一, 主要用于传递用户自己的参数,
 *  用户需要在 callback 内将该参数转换成自己的可识别类型
 */
ACL_API void acl_ioctl_enable_read(ACL_IOCTL *ioc, ACL_VSTREAM *stream,
	int timeout, ACL_IOCTL_NOTIFY_FN callback, void *context);

/**
 * 向任务池中添加一个写监控工作任务
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @param stream {ACL_VSTREAM*} 客户端数据流指针
 * @param timeout {int} 流连接空闲超时时间
 * @param callback {ACL_IOCTL_NOTIFY_FN} 当数据流可写或出错或超时时的回调函数
 * @param context {void*} 回调函数 callback 的参数之一, 主要用于传递用户自己的参数,
 *  用户需要在 callback 内将该参数转换成自己的可识别类型
 */
ACL_API void acl_ioctl_enable_write(ACL_IOCTL *ioc, ACL_VSTREAM *stream,
	int timeout, ACL_IOCTL_NOTIFY_FN callback, void *context);

/**
 * 异步地连接服务器, 连接成功或连接超时时后调用用户的回调函数
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @param stream {ACL_VSTREAM*} 处于连接远程服务器状态的本地客户端数据流
 * @param timeout {int} 连接超时时间
 * @param callback {ACL_IOCTL_NOTIFY_FN} 当数据流可写或出错或超时时的回调函数
 * @param context {void*} 回调函数 callback 的参数之一, 主要用于传递用户自己的参数,
 *  用户需要在 callback 内将该参数转换成自己的可识别类型
 */
ACL_API void acl_ioctl_enable_connect(ACL_IOCTL *ioc, ACL_VSTREAM *stream,
	int timeout, ACL_IOCTL_NOTIFY_FN callback, void *context);

/**
 * 作为服务端来监听某个待监听地址
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @param stream {ACL_VSTREAM*} 处于连接远程服务器状态的本地客户端数据流
 * @param timeout {int} 监听套接字监听超时时间, 当此超时时间到达且没有新连接到达时,
 *  调用者可以在回调函数里处理其它事件, 如果该值为 0 则一直阻塞
 *  到有新连接到达或出错时用户的回调函数才被调用
 * @param callback {ACL_IOCTL_NOTIFY_FN} 当有新连接到达或监听套接字出错
 *  或监听超时时的回调函数
 * @param context {void*} callback 的参数之一, 参见上面说明
 */
ACL_API void acl_ioctl_enable_listen(ACL_IOCTL *ioc, ACL_VSTREAM *stream,
	int timeout, ACL_IOCTL_NOTIFY_FN callback, void *context);

/*----------------------------------------------------------------------------*/
/**
 * 连接远程服务器
 * @param addr {const char*} 服务器端服务地址, 格式: ip:port, 如: 192.168.0.1:80
 * @param timeout {int} 连接超时时间, 其含义如下:
 *         1) 0:   非阻塞地连接远程服务器
 *         2) -1:  阻塞地连接远程服务器直至连接成功或连接失败为止
 *         3) > 0: 带超时地连接远程服务器
 * @return {ACL_VSTREAM*} 客户端连接流.
 *  != NULL 表示连接成功或正在连接中; == NULL 连接失败或出错
 * 注:
 *     当处于上述情况 1) 时, 需要将返回的 ACL_VSTREAM 句柄置入可连接集合中通过回调
 *     函数来对该流进行读写操作, 即还需要调用 acl_ioctl_enable_connect() 来确保连
 *     接成功.
 */
ACL_API ACL_VSTREAM *acl_ioctl_connect(const char *addr, int timeout);

/**
 * 创建一个监听套接字流
 * @param addr {const char*} 本地被监听的地址, 格式: ip:port, 如: 127.0.0.1:80
 * @param qlen {int} 监听队列长度
 * @return {ACL_VSTREAM*} 监听套接字数据流. != NULL ok; == NULL err.
 * 注: 若要异步监听, 则可以调用 acl_ioctl_enable_listen() 来异步地
 *     获得一个客户端连接
 */
ACL_API ACL_VSTREAM *acl_ioctl_listen(const char *addr, int qlen);

/**
 * 创建一个监听套接字流
 * @param addr {const char*} 本地被监听的地址, 格式: ip:port, 如: 127.0.0.1:80
 * @param qlen {int} 监听队列长度
 * @param block_mode {int} 是否采用非阻塞模式, ACL_BLOCKING: 阻塞模式,
 *  ACL_NON_BLOCKING: 非阻塞模式
 * @param io_bufsize {int} 获得客户端连接流的缓冲区大小(字节)
 * @param io_timeout {int} 客户端流的读写超时时间(秒)
 * @return {ACL_VSTREAM*} 监听套接字数据流. != NULL ok; == NULL err.
 * 注: 若要异步监听, 则可以调用 acl_ioctl_enable_listen() 来异步地
 *     获得一个客户端连接
 */
ACL_API ACL_VSTREAM *acl_ioctl_listen_ex(const char *addr, int qlen,
	int block_mode, int io_bufsize, int io_timeout);

/**
 * 从监听套口获得一个客户端连接
 * @param sstream {ACL_VSTREAM*} 监听套字流
 * @param ipbuf {char*} 客户端流的地址
 * @param size {int} ipbuf 空间大小
 * @return {ACL_VSTREAM*} 客户端连接流. != NULL 成功获得一个客户端连接的数据流;
 *  == NULL 可能被系统中断了一下, 调用者应忽略此情况
 */
ACL_API ACL_VSTREAM *acl_ioctl_accept(ACL_VSTREAM *sstream,
	char *ipbuf, int size);

/**
 * 给任务工作池添加一个定时器任务, 该函数仅是 acl_event_request_timer 的简单封装.
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @param timer_fn {ACL_EVENT_NOTIFY_TIME} 定时器任务回调函数.
 * @param context {void*} timer_fn 的参数之一.
 * @param idle_limit {acl_int64} 启动定时器函数的时间(微秒级)
 * @return {acl_int64} 剩余的时间, 单位为微秒.
 */
ACL_API acl_int64 acl_ioctl_request_timer(ACL_IOCTL *ioc,
	ACL_EVENT_NOTIFY_TIME timer_fn, void *context, acl_int64 idle_limit);

/**
 * 取消某个定时器任务, 该函数仅是 acl_event_cancel_timer 的简单封装.
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @param timer_fn {ACL_EVENT_NOTIFY_TIME} 定时器任务回调函数.
 * @param context {void*} timer_fn 的参数之一.
 * @return {acl_int64} 剩余的时间, 单位为微秒.
 */
ACL_API acl_int64 acl_ioctl_cancel_timer(ACL_IOCTL *ioc,                                                            
	ACL_EVENT_NOTIFY_TIME timer_fn, void *context);

/**
 * 向当前线程池中增加一个新的任务
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @param callback {ACL_IOCTL_WORKER_FN} 工作任务的回调函数
 * @param arg {void*} callback 的参数之一
 * @return {int} 0: ok; < 0: error
 */
ACL_API int acl_ioctl_add(ACL_IOCTL *ioc,
	ACL_IOCTL_WORKER_FN callback, void *arg);

/**
 * 获得当前线程池中工作线程的数量.
 * @param ioc {ACL_IOCTL*} 服务器任务池句柄
 * @return {int} 返回当前工作线程的数量 == -1: error; >= 0: ok.
 */
ACL_API int acl_ioctl_nworker(ACL_IOCTL *ioc);

#ifdef	__cplusplus
}
#endif

#endif
