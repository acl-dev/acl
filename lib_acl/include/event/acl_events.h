#ifndef ACL_EVENTS_H_INCLUDED
#define ACL_EVENTS_H_INCLUDED

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#include <time.h>
#include "stdlib/acl_vstream.h"
#include "acl_timer.h"

/*+++++++++++++++++++++++++++ 全局宏定义 +++++++++++++++++++++++++++++++++++*/
 /* Event codes. */
#define ACL_EVENT_READ          (1 << 0)      /**< read event */
#define	ACL_EVENT_ACCEPT        (1 << 1)      /**< accept one connection */
#define ACL_EVENT_WRITE         (1 << 2)      /**< write event */
#define	ACL_EVENT_CONNECT       (1 << 3)      /**< client has connected the server*/
#define ACL_EVENT_XCPT          (1 << 4)      /**< exception */
#define ACL_EVENT_TIME          (1 << 5)      /**< timer event */
#define	ACL_EVENT_RW_TIMEOUT    (1 << 6)      /**< read/write timeout event */
#define	ACL_EVENT_TIMEOUT       ACL_EVENT_RW_TIMEOUT

#define	ACL_EVENT_FD_IDLE	0
#define	ACL_EVENT_FD_BUSY	1

#define ACL_EVENT_ERROR		ACL_EVENT_XCPT

#define	ACL_EVENT_SELECT	0
#define	ACL_EVENT_POLL		1
#define	ACL_EVENT_KERNEL	2
#define ACL_EVENT_WMSG		3

 /*
  * Dummies.
  */
#define ACL_EVENT_NULL_TYPE	0
#define ACL_EVENT_NULL_CONTEXT	((char *) 0)


/* in acl_events.c */
/*
 * Timer events. Timer requests are kept sorted, in a circular list. We use
 * the RING abstraction, so we get to use a couple ugly macros.
 */
typedef struct ACL_EVENT_TIMER ACL_EVENT_TIMER;

typedef	struct	ACL_EVENT		ACL_EVENT;
typedef	struct	ACL_EVENT_FDTABLE	ACL_EVENT_FDTABLE;

/*
 * External interface.
 */
#if 0
typedef void (*ACL_EVENT_NOTIFY_FN) (int event_type, void *context);
typedef	ACL_EVENT_NOTIFY_FN	ACL_EVENT_NOTIFY_RDWR;
typedef	ACL_EVENT_NOTIFY_FN	ACL_EVENT_NOTIFY_TIME;
#else
typedef void (*ACL_EVENT_NOTIFY_RDWR)(int event_type, ACL_EVENT *event,
		ACL_VSTREAM *stream, void *context);
typedef void (*ACL_EVENT_NOTIFY_TIME)(int event_type, ACL_EVENT *event,
		void *context);
#endif

/*----------------------------------------------------------------------------*/

/**
 * 创建一个事件循环对象的总入口，此函数会根据用户参数的不同自动调用下面的事件对象创建函数
 * @param event_mode {int} 事件处理方式，目前仅支持: ACL_EVENT_SELECT, ACL_EVENT_KERNEL,
 *  ACL_EVENT_POLL, ACL_EVENT_WMSG
 * @param use_thr {int} 是否采用线程事件方式，非0表示按线程事件方式
 * @param delay_sec {int} 事件循环等待时的最长秒数，当 event_mode 为 ACL_EVENT_WMSG
 *  时，且该值大于 0 时，则该值被当作消息值对待传给 acl_event_new_wmsg，用来与异
 *  步消息句柄绑定
 * @param delay_usec {int} 事件循环等待时的最长微秒数(仅 select 方式有用)
 * @return {ACL_EVENT*} 事件对象指针，如果为空表示出错
 */
ACL_API ACL_EVENT *acl_event_new(int event_mode, int use_thr,
	int delay_sec, int delay_usec);

/**
 * 创建一个新的事件对象, 该事件不支持多线程
 * @param delay_sec {int} 在调用 select() 函数时休息的秒数
 * @param delay_usec {int} 在调用 select() 函数时休息的微秒数
 * @return {ACL_EVENT*} 事件对象指针，如果为空表示出错
 */
ACL_API ACL_EVENT *acl_event_new_select(int delay_sec, int delay_usec);

/**
 * 创建一个新的事件对象, 该事件支持线程模式
 * @param delay_sec {int} 在调用 select() 函数时休息的秒数
 * @param delay_usec {int} 在调用 select() 函数时休息的微秒数
 * @return {ACL_EVENT*} 事件对象指针，如果为空表示出错
 */
ACL_API ACL_EVENT *acl_event_new_select_thr(int delay_sec, int delay_usec);

/**
 * 创建一个支持 poll 的事件对象，不支持多线程
 * @param delay_sec {int} 在调用 poll() 函数时休息的秒数
 * @param delay_usec {int} 在调用 poll() 函数时休息的微秒数
 * @return {ACL_EVENT*} 事件对象指针，如果为空表示出错
 */
ACL_API ACL_EVENT *acl_event_new_poll(int delay_sec, int delay_usec);

/**
 * 创建一个支持 poll 的事件对象，支持多线程
 * @param delay_sec {int} 在调用 poll() 函数时休息的秒数
 * @param delay_usec {int} 在调用 poll() 函数时休息的微秒数
 * @return {ACL_EVENT*} 事件对象指针，如果为空表示出错
 */
ACL_API ACL_EVENT *acl_event_new_poll_thr(int delay_sec, int delay_usec);

/**
 * 创建一个新的事件对象, 该事件采用效率高的 epoll/devpoll/kqueue 方式，且不支持多线程
 * @param delay_sec {int} 在调用事件循环函数时休息的秒数
 * @param delay_usec {int} 在调用事件循环函数时休息的微秒数(忽略不计)
 * @return {ACL_EVENT*} 事件对象指针，如果为空表示出错
 */
ACL_API ACL_EVENT *acl_event_new_kernel(int delay_sec, int delay_usec);

/**
 * 创建一个新的事件对象, 该事件采用效率高的 epoll/devpoll/kqueue 方式，且采用线程方式
 * @param delay_sec {int} 在调用事件循环函数时休息的秒数
 * @param delay_usec {int} 在调用事件循环函数时休息的微秒数(忽略不计)
 * @return {ACL_EVENT*} 事件对象指针，如果为空表示出错
 */
ACL_API ACL_EVENT *acl_event_new_kernel_thr(int delay_sec, int delay_usec);

/**
 * 创建一个能与 Windows 界面消息绑在一起的事件引擎对象
 * @param nMsg {unsigned int} 如果该值大于 0 则将该异步句柄与该消息值绑定，
 *  否则将该异步句柄与缺省的消息值绑定
 * @return {ACL_EVENT*} 事件对象指针，如果为空表示出错
 */
ACL_API ACL_EVENT *acl_event_new_wmsg(unsigned int nMsg);

#if defined (_WIN32) || defined(_WIN64)
ACL_API HWND acl_event_wmsg_hwnd(ACL_EVENT *eventp);
#endif

/**
 * 为了防止在多线程模式下 select 等事件循环的时间等待，可以添加此项以中断等待，
 * 加快事件循环过程
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 */
ACL_API void acl_event_add_dog(ACL_EVENT *eventp);

/**
 * 设置事件触发的前置和后置处理过程
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param fire_begin {void (*)(ACL_EVENT*, void*)} 当事件被统一触发前的回调过程
 * @param fire_end {void (*)(ACL_EVENT*, void*)} 当事件被统一触发后的回调过程
 * @param ctx {void*} fire_begin / fire_end 的第二个参数
 */
ACL_API void acl_event_set_fire_hook(ACL_EVENT *eventp,
		void (*fire_begin)(ACL_EVENT*, void*),
		void (*fire_end)(ACL_EVENT*, void*),
		void* ctx);

/**
 * 设置事件循环过程中定时检查所有描述字状态的时间间隔，内部缺省值为 100 ms
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param n {int} 定时查检时间间隔 (毫秒级)
 */
ACL_API void acl_event_set_check_inter(ACL_EVENT *eventp, int n);

/**
 * 释放事件结构
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 */
ACL_API void acl_event_free(ACL_EVENT *eventp);

/**
 * 返回事件的时间截
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @return {acl_int64} 当前事件的时间截(微秒级别)
 */
ACL_API acl_int64 acl_event_time(ACL_EVENT *eventp);

/**
 * 将事件中的所有任务执行完毕
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 */
ACL_API void acl_event_drain(ACL_EVENT *eventp);

/**
 * 设置数据流可读时(指有数据待读或描述符出错或描述符关闭)的回调函数
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param stream {ACL_VSTREAM*} 数据流指针, 不能为空, 且其中的描述符必须是有效的
 * @param read_timeout {int} 读超时时间(秒)
 * @param callback {ACL_EVENT_NOTIFY_RDWR} 数据流可读时的回调函数
 * @param context {void*} 回调函数 callback 所需要的参数
 */
ACL_API void acl_event_enable_read(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int read_timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context);

/**
 * 设置数据流可写时(指有空间可以写或描述符出错或描述符关闭)的回调函数
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param stream {ACL_VSTREAM*} 数据流指针, 不能为空, 且其中的描述符必须是有效的
 * @param write_timeout {int} 写超时时间(秒)
 * @param callback {ACL_EVENT_NOTIFY_RDWR} 数据流可写时的回调函数
 * @param context {void*} 回调函数 callback 所需要的参数
 */
ACL_API void acl_event_enable_write(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int write_timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context);

/**
 * 设置监听套接口(指有新连接到达/被系统中断/出错时)的回调函数
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param stream {ACL_VSTREAM*} 数据流指针, 不能为空, 且其中的描述符必须是有效的
 * @param read_timeout {int} 监听超时时间(秒)，可以为0
 * @param callback {ACL_EVENT_NOTIFY_RDWR} 数据流可读时的回调函数
 * @param context {void*} 回调函数 callback 所需要的参数
 */
ACL_API void acl_event_enable_listen(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int read_timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context);

/**
 * 将数据流从事件的读监听流集合中清除
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param stream {ACL_VSTREAM*} 数据流指针, 不能为空, 且其中的描述符必须是有效的
 */
ACL_API void acl_event_disable_read(ACL_EVENT *eventp, ACL_VSTREAM *stream);

/**
 * 将数据流从事件的写监听流集合中清除
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param stream {ACL_VSTREAM*} 数据流指针, 不能为空, 且其中的描述符必须是有效的
 */
ACL_API void acl_event_disable_write(ACL_EVENT *eventp, ACL_VSTREAM *stream);

/**
 * 将数据流从事件的读写监听流集合中清除
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param stream {ACL_VSTREAM*} 数据流指针, 不能为空, 且其中的描述符必须是有效的
 */
ACL_API void acl_event_disable_readwrite(ACL_EVENT *eventp, ACL_VSTREAM *stream);

/**
 * 检查流中的描述符是否已经置入读、写或异常事件的集合中
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param stream {ACL_VSTREAM*} 数据流指针, 不能为空, 且其中的描述符必须是有效的
 */
ACL_API int acl_event_isset(ACL_EVENT *eventp, ACL_VSTREAM *stream);

/**
 * 检查流中的描述符是否已经置入读事件的集合中
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param stream {ACL_VSTREAM*} 数据流指针, 不能为空, 且其中的描述符必须是有效的
 */
ACL_API int acl_event_isrset(ACL_EVENT *eventp, ACL_VSTREAM *stream);

/**
 * 检查流中的描述符是否已经置入写事件的集合中
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param stream {ACL_VSTREAM*} 数据流指针, 不能为空, 且其中的描述符必须是有效的
 */
ACL_API int acl_event_iswset(ACL_EVENT *eventp, ACL_VSTREAM *stream);

/**
 * 检查流中的描述符是否已经置入异常事件的集合中
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param stream {ACL_VSTREAM*} 数据流指针, 不能为空, 且其中的描述符必须是有效的
 */
ACL_API int acl_event_isxset(ACL_EVENT *eventp, ACL_VSTREAM *stream);

/**
 * 添加一个定时事件
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param callback {ACL_EVENT_NOTIFY_TIME} 定时事件的回调函数
 * @param context {void*} callback 所需要的回调参数
 * @param delay {acl_int64} eventp->event_present + delay 为该事件函数开始执行的时间
 *  单位为微秒
 * @param keep {int} 是否重复定时器任务
 * @return {acl_int64} 事件执行的时间截，单位为微秒
 */
ACL_API acl_int64 acl_event_request_timer(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback, void *context, acl_int64 delay, int keep);

/**
 * 取消一个定时事件
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param callback {ACL_EVENT_NOTIFY_TIME} 定时事件的回调函数
 * @param context {void*} callback 所需要的回调参数
 * @return acl_int64 {acl_int64} 距离开始执行事件函数的时间间隔, 以微秒为单位
 */
ACL_API acl_int64 acl_event_cancel_timer(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback, void *context);

/**
 * 当定时器处理完毕后，是否需要再次设置该定时器，以方便调用者循环
 * 使用该定时器
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param callback {ACL_EVENT_NOTIFY_TIME} 非空
 * @param context {void*} 附属于 callback 的变量
 * @param onoff {int} 是否重复通过 acl_event_request_timer 设置的定时器
 */
ACL_API void acl_event_keep_timer(ACL_EVENT *eventp, ACL_EVENT_NOTIFY_TIME callback,
	void *context, int onoff);

/**
 * 判断所设置的定时器都处于重复使用状态
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param callback {ACL_EVENT_NOTIFY_TIME} 非空
 * @param context {void*} 附属于 callback 的变量
 * @return {int} !0 表示所设置的定时器都处于重复使用状态
 */
ACL_API int acl_event_timer_ifkeep(ACL_EVENT *eventp, ACL_EVENT_NOTIFY_TIME callback,
	void *context);

/**
 * 事件循环执行的调度函数
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 */
ACL_API void acl_event_loop(ACL_EVENT *eventp);

/**
 * 设置事件循环的空闲休息时间中的秒级数值
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param sec {int} 秒级空闲休息时间值
 */
ACL_API void acl_event_set_delay_sec(ACL_EVENT *eventp, int sec);

/**
 * 设置事件循环的空闲休息时间中的微秒级数值
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @param usec {int} 微秒级空闲休息时间值
 */
ACL_API void acl_event_set_delay_usec(ACL_EVENT *eventp, int usec);

/**
 * 是否采用线程事件方式
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @return {int} 0: 否; !=0: 是
 */
ACL_API int acl_event_use_thread(ACL_EVENT *eventp);

/**
 * 获得当前事件引擎的事件模型
 * @param eventp {ACL_EVENT*} 事件对象指针, 不为能为空
 * @return {int} ACL_EVENT_SELECT/ACL_EVENT_KERNEL/ACL_EVENT_POLL
 */
ACL_API int acl_event_mode(ACL_EVENT *eventp);

#ifdef	__cplusplus
}
#endif

#endif
