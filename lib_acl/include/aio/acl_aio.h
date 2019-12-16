/**
 * @file	acl_aio.h
 * @author	zsx
 * @date	2010-1-2
 * @brief	本文件中定义了关于 ACL_ASTREAM　异步通信流操作的类型说明及函数接口.
 * @version	1.1
 */

#ifndef	ACL_AIO_INCLUDE_H
#define	ACL_AIO_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include <stdarg.h>
#ifdef	ACL_UNIX
#include <sys/uio.h>
#endif

#include "../stdlib/acl_stdlib.h"
#include "../event/acl_events.h"
#include "../net/acl_netdb.h"

/*------------------------------- 数据结构类型定义 ---------------------------*/

/**
 * 异步框架引擎句柄类型定义
 */
typedef struct ACL_AIO ACL_AIO;

/**
 * 异步流类型定义
 */
typedef struct ACL_ASTREAM ACL_ASTREAM;

/**
 * 事件通知函数句柄类型, 当某个受监控的流有数据可读或出错时的回调用户的注册函数,
 * 目前用与该类型相关的异步函数有:
 *   acl_aio_gets, acl_aio_gets_nonl, acl_aio_read, acl_aio_readn.
 * @param astream {ACL_ASTREAM*} 异步流指针
 * @param context {void*} 用户级传递的参数
 * @param data {const char*} 从流中读取的数据指针
 * @param dlen {int} data 数据的长度
 * @return {int} 该函数指针调用如果返回-1则表明应用要求关闭异步流
 */
typedef int (*ACL_AIO_READ_FN)(ACL_ASTREAM *astream,
	void *context, char *data, int dlen);

/**
 * 事件通知函数句柄类型，当某个异步流可读/可写时调用此类型的用户回调函数
 * @param astream {ACL_ASTREAM*} 异步流指针
 * @param context {void*} 用户级传递的参数
 * @return {int} 如果该函数类型返回 -1 则表明应用要求关闭异步流
 */
typedef int (*ACL_AIO_NOTIFY_FN)(ACL_ASTREAM *astream, void *context);

/**
 * 事件通知函数句柄类型, 当某个受监控的流将数据写完或出错时的回调用户的注册函数,
 * 目前用与该类型相关的异步函数有:
 *   acl_aio_writen, acl_aio_writev, acl_aio_fprintf, acl_aio_vfprintf.
 * @param astream {ACL_ASTREAM*} 异步流指针
 * @param context {void*} 用户级传递的参数
 * @return {int} 该函数指针调用如果返回-1则表明应用要求关闭异步流
 */
typedef int (*ACL_AIO_WRITE_FN)(ACL_ASTREAM *astream, void *context);

/**
 * 当某个监听描述符有新的客户端连接时, 异步框架接收该连接并传递给用户; 如果出错,
 * 若用户设置了该监听流的监听超时值且到达该超时值, 则也会触发该函数类型句柄. 与该函数
 * 类型句柄相关的异步函数有: acl_aio_accept.
 * @param cstream {ACL_ASTREAM*} 从 sstream 监听流通过 accept() 获得的客户端连接流
 * @param context {void*} 用户级传递的参数
 * @return {int} 如果该函数调用返回 -1 表示不再继续接收新的客户端连接
 */
typedef int (*ACL_AIO_ACCEPT_FN)(ACL_ASTREAM *cstream,	void *context);

/**
 * 当某个监听描述符上有新的客户端连接时, 异步框架回调用用户的注册函数, 用户需要从
 * 该监听流上 accept 该客户端连接. 与该函数类型相关的异步函数有: acl_aio_listen.
 * @param sstream {ACL_ASTREAM*} 监听流句柄
 * @param context {void*} 用户级传递的参数
 * @return {int} 如果该函数的调用返回-1，并不影响监听流继续监听
 * 注: 请注意该函数类型与 ACL_AIO_ACCEPT_FN 的功能差别.
 */
typedef int (*ACL_AIO_LISTEN_FN)(ACL_ASTREAM *sstream, void *context);

/**
 * 异步连接远程服务器时, 当连接失败、超时或成功时的事件通知句柄类型
 * 将回调用户的注册函数. 与该函数类型相关的异步函数有: acl_aio_connect.
 * @param cstream {ACL_ASTREAM*} 受监控的正处于连接状态的客户端流
 * @param context {void*} 用户级传递的参数
 * @return {int} 若调用该函数返回-1则需要关闭该异步连接流
 */
typedef int (*ACL_AIO_CONNECT_FN)(ACL_ASTREAM *cstream, void *context);

typedef struct ACL_ASTREAM_CTX ACL_ASTREAM_CTX;

ACL_API int acl_astream_get_status(const ACL_ASTREAM_CTX *ctx);
#define ACL_ASTREAM_STATUS_INVALID		-1
#define ACL_ASTREAM_STATUS_OK			0
#define ACL_ASTREAM_STATUS_NS_ERROR		1
#define ACL_ASTREAM_STATUS_CONNECT_ERROR	2
#define ACL_ASTREAM_STATUS_CONNECT_TIMEOUT	3

ACL_API const ACL_SOCKADDR *acl_astream_get_ns_addr(const ACL_ASTREAM_CTX *ctx);
ACL_API const ACL_SOCKADDR *acl_astream_get_serv_addr(const ACL_ASTREAM_CTX *ctx);
ACL_API ACL_ASTREAM *acl_astream_get_conn(const ACL_ASTREAM_CTX *ctx);
ACL_API void *acl_astream_get_ctx(const ACL_ASTREAM_CTX *ctx);

/**
 * 异步连接远程服务器时的回调函数定义，该类型由 acl_aio_connect_addr() 使用
 * @param ctx {ACL_ASTREAM_CTX*} 回调函数的参数，可以由 acl_astream_get_xxx
 *  获得该对象中包含的对象指针
 */
typedef int (*ACL_AIO_CONNECT_ADDR_FN)(const ACL_ASTREAM_CTX *ctx);

/**
 * “读、写、监听”超时的回调函数指针
 * @param astream {ACL_ASTREAM*} 异步流指针
 * @param context {void*} 用户传递的参数
 * @return {int} 当该函数调用返回-1时，对于读写流表示需要关闭该异步读写流，
 *  对于监听流表示不再继续接收新的客户端连接；当返回0时，表示继续
 */
typedef int (*ACL_AIO_TIMEO_FN)(ACL_ASTREAM *astream, void *context);

/**
 * 当需要关闭异步读写流时需要回调用用户注册的函数
 * @param astream {ACL_ASTREAM*} 异步流指针
 * @param context {void*} 用户传递的参数
 * @return {int} 无论该值如何，该异步流都需要被关闭
 */
typedef int (*ACL_AIO_CLOSE_FN)(ACL_ASTREAM *astream, void *context);

/* 异步流类型定义 */

struct ACL_ASTREAM {
	ACL_AIO *aio;		/**< 异步流事件句柄 */
	ACL_VSTREAM *stream;	/**< 同步流 */

	ACL_VSTRING strbuf;	/**< 内部缓冲区 */
	int   timeout;		/**< IO超时时间 */
	int   nrefer;		/**< 通过此引用计数防止流被提前关闭 */
	int   flag;		/**< 标志位 */
#define ACL_AIO_FLAG_IOCP_CLOSE     (1 << 0)
#define	ACL_AIO_FLAG_ISRD           (1 << 1)
#define	ACL_AIO_FLAG_ISWR           (1 << 2)
#define ACL_AIO_FLAG_DELAY_CLOSE    (1 << 3)
#define ACL_AIO_FLAG_DEAD           (1 << 4)

	ACL_FIFO write_fifo;	/**< 异步写时的先进先出队列数据 */
	int   write_left;	/**< 写缓冲中未写完的数据量 */
	int   write_offset;	/**< 写缓冲中的下一个位置偏移 */
	int   write_nested;	/**< 写时的嵌套层数 */
	int   write_nested_limit;  /**< 写时的嵌套层数限制 */

	int   (*read_ready_fn) (ACL_VSTREAM *, ACL_VSTRING *, int *);
	int   read_nested;	/**< 读时的嵌套层数 */
	int   read_nested_limit;  /**< 读时的嵌套层数限制 */
	int   count;		/**< 调用 acl_aio_readn()/2 时设置的第二个参数值 */
	int   keep_read;	/**< 是否启用持续性读 */
	int   accept_nloop;	/**<  acl_aio_accept 内部循环 accept 的最大次数 */
	int   error;		/**< 当前套接口的错误号 */
	int   line_length;	/**< 当以行为单位读数据时该值限制每行最大长度 */

	ACL_AIO_ACCEPT_FN  accept_fn;	/**< accept 完成时的回调函数 */
	ACL_AIO_LISTEN_FN  listen_fn;	/**< 有新连接到达时的回调函数 */
	void *context;			/**< 用户设置的参数 */

	ACL_AIO_NOTIFY_FN  can_read_fn; /**< 可以读时的回调函数 */
	void *can_read_ctx;		/**< can_read_fn 参数之一 */
	ACL_AIO_NOTIFY_FN  can_write_fn; /**< 可以写时的回调函数 */
	void *can_write_ctx;		/**< can_write_fn 参数之一 */

	ACL_ARRAY *read_handles;	/**< 读完成时的辅助回调函数 */
	ACL_ARRAY *write_handles;	/**< 写完成时的辅助回调函数 */
	ACL_ARRAY *close_handles;	/**< 关闭时的辅助回调函数 */
	ACL_ARRAY *timeo_handles;	/**< 超时时的辅助回调函数 */
	ACL_ARRAY *connect_handles;	/**< 连接成功时辅助回调函数 */
	ACL_FIFO   reader_fifo;		/**< 临时存放回调函数 */
	ACL_FIFO   writer_fifo;		/**< 临时存放回调函数 */

	/* 可读时的回调函数 */
	void (*event_read_callback)(int event_type, ACL_ASTREAM *astream);
};

/**
 * 设置流的IO超时时间
 */
#define ACL_AIO_SET_TIMEOUT(stream_ptr, _timeo_) do {  \
	ACL_ASTREAM *__stream_ptr = stream_ptr;        \
	__stream_ptr->timeout = _timeo_;               \
} while(0)

/**
 * 设置流的 context 参数
 */
#define ACL_AIO_SET_CTX(stream_ptr, _ctx_) do {  \
	ACL_ASTREAM *__stream_ptr = stream_ptr;  \
	__stream_ptr->context = _ctx_;           \
} while(0)

/*--------------------------- 异步操作公共接口 -------------------------------*/

/**
 * 创建一个异步通信的异步框架实例句柄, 可以指定是否采用 epoll/devpoll
 * @param event_mode {int} 事件监听方式: ACL_EVENT_SELECT, ACL_EVENT_POLL
 *  , ACL_EVENT_KERNEL, ACL_EVENT_WMSG
 * @return {ACL_AIO*} 返回一个异步框架引擎句柄. OK: != NULL; ERR: == NULL.
 */
ACL_API ACL_AIO *acl_aio_create(int event_mode);

/**
 * 创建异步框架实例句柄, 可以指定是否采用 epoll/devpoll/windows message
 * @param event_mode {int} 事件监听方式: ACL_EVENT_SELECT, ACL_EVENT_POLL
 *  , ACL_EVENT_KERNEL, ACL_EVENT_WMSG
 * @param nMsg {unsigned int} 当与 _WIN32 界面的消息整合时，即 event_mode 设为
 *  ACL_EVENT_WMSG 时该值才有效，其表示与异步句柄绑定的消息值
 * @return {ACL_AIO*} 返回一个异步框架引擎句柄. OK: != NULL; ERR: == NULL.
 */
ACL_API ACL_AIO *acl_aio_create2(int event_mode, unsigned int nMsg);

/**
 * 根据事件引擎句柄创建异步对象句柄
 * @param event {ACL_EVENT *}
 * @return {ACL_AIO *}
 */
ACL_API ACL_AIO *acl_aio_create3(ACL_EVENT *event);

/**
 * 获得本 aio 句柄所绑定的 DNS 查询对象
 * @param aio {ACL_AIO*}
 * @return {ACL_DNS*} 返回 NULL 表示没有绑定 DNS 查询对象，当返回值非 NULL 时，应用可
 *  以直接将返回值转换为 ACL_DNS 对象（XXX：因为循环引用头文件的问题，所以暂且如此）
 */
ACL_API void *acl_aio_dns(ACL_AIO *aio);

/**
 * 设置 DNS 服务器地址列表，只有设置了 DNS 服务器地址，内部才会支持域名解析并
 * 异步连接服务器地址
 * @param aio {ACL_AIO*}
 * @param dns_list {const char*} DNS 服务器地址列表，格式：ip1:port,ip2:port...
 * @param timeout {int} 域名解析超时时间（秒）
 * @return {int} 设置 DNS 查询对象是否成功，0 表示成功，-1 表示失败，失败原因
 *  有：无法创建 UDP 套接字或绑定 UDP 套接字失败
 */
ACL_API int acl_aio_set_dns(ACL_AIO *aio, const char *dns_list, int timeout);

/**
 * 删除 DNS 服务器地址列表
 * @param aio {ACL_AIO*}
 * @param dns_list {const char*} DNS 服务器地址列表，格式：ip1:port,ip2:port...
 */
ACL_API void acl_aio_del_dns(ACL_AIO *aio, const char *dns_list);

/**
 * 将 aio 句柄中绑定的 DNS 地址清理掉
 * @param aio {ACL_AIO*}
 */
ACL_API void acl_aio_clear_dns(ACL_AIO *aio);

/**
 * 释放一个异步通信异步框架实例句柄，同时会释放掉非空的 aio->event 对象
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 */
ACL_API void acl_aio_free(ACL_AIO *aio);

/**
 * 释放一个异步通信异步框架实例句柄
 * @param keep {int} 是否同时释放掉 aio 所绑定的 event 事件句柄
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 */
ACL_API void acl_aio_free2(ACL_AIO *aio, int keep);

/**
 * 异步IO消息循环(仅在单线程模式下调用)
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 */
ACL_API void acl_aio_loop(ACL_AIO *aio);

/**
 * 获得本次事件循环被触发的事件次数
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 * @return {int} -1 表示输入参数有误，否则返回值 >= 0
 */
ACL_API int acl_aio_last_nready(ACL_AIO *aio);

/**
 * 主动检查 ACL_AIO 引擎中待关闭的异步流是否应该关闭，调用此函数后，一些需要
 * 延迟关闭的异步流会被主动关闭
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 */
ACL_API void acl_aio_check(ACL_AIO *aio);

/**
 * 获得事件引擎的句柄
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 * @return {ACL_EVENT*}
 */
ACL_API ACL_EVENT *acl_aio_event(ACL_AIO *aio);

/**
 * 获得事件所采用的模式
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 * @return {int} ACL_EVENT_KERNEL/ACL_EVENT_SELECT/ACL_EVENT_POLL
 */
ACL_API int acl_aio_event_mode(ACL_AIO *aio);

/**
 * 异步IO框架是否是采用持续读模式
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 * @return {int} != 0: 是; == 0: 否
 */
ACL_API int acl_aio_get_keep_read(ACL_AIO *aio);

/**
 * 设置异步IO框架的持续读模式
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 * @param onoff {int} 0: 关闭持续读功能; != 0: 打开持续读功能
 */
ACL_API void acl_aio_set_keep_read(ACL_AIO *aio, int onoff);

/**
 * 获得当前异步引擎循环时的等待时间的秒部分
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 * @return {int} 用 select/poll/epoll/kqueue/devpoll 时的秒级等待时间
 */
ACL_API int acl_aio_get_delay_sec(ACL_AIO *aio);

/**
 * 获得当前异步引擎循环时的等待时间的微秒部分
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 * @return {int} 用 select/poll/epoll/kqueue/devpoll 时的微秒级等待时间
 */
ACL_API int acl_aio_get_delay_usec(ACL_AIO *aio);

/**
 * 设置异步引擎循环的等待时间中的秒级部分
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 * @param delay_sec {int} 设置用 select/poll/epoll/kqueue/devpoll
 *  时的秒级等待时间
 */
ACL_API void acl_aio_set_delay_sec(ACL_AIO *aio, int delay_sec);

/**
 * 设置异步引擎循环的等待时间中的微秒级部分
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 * @param delay_usec {int} 设置用 select/poll/epoll/kqueue/devpoll
 *  时的微秒级等待时间
 */
ACL_API void acl_aio_set_delay_usec(ACL_AIO *aio, int delay_usec);

/**
 * 设置事件循环过程中定时检查所有描述字状态的时间间隔，内部缺省值为 100 ms
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 * @param check_inter {int} 定时查检时间间隔 (毫秒级)
 */
ACL_API void acl_aio_set_check_inter(ACL_AIO *aio, int check_inter);

/**
 * 设置异步流的读缓存区大小
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 * @param rbuf_size {int} 读缓冲区大小
 */
ACL_API void acl_aio_set_rbuf_size(ACL_AIO *aio, int rbuf_size);

/**
 * 设置监听异步流每次接收客户端连接时循环接收个数
 * @param astream {ACL_ASTREAM*} 监听流
 * @param nloop {int}
 */
ACL_API void acl_aio_set_accept_nloop(ACL_ASTREAM *astream, int nloop);

/**
 * 从异步流中获得异步框架引擎句柄
 * @param stream {ACL_ASTREAM*} 异步IO流
 * @return {ACL_AIO*} 异步框架引擎句柄
 */
ACL_API ACL_AIO *acl_aio_handle(ACL_ASTREAM *stream);

/**
 * 设置异步流的参数
 * @param stream {ACL_ASTREAM*} 异步IO流
 * @param ctx {void*} 参数
 */
ACL_API void acl_aio_set_ctx(ACL_ASTREAM *stream, void *ctx);

/**
 * 获得异步流的参数
 * @param stream {ACL_ASTREAM*} 异步IO流
 * @return {void*} 异步流 stream 的参数
 */
ACL_API void *acl_aio_get_ctx(ACL_ASTREAM *stream);

/**
 * 打开一个异步通信流的句柄
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 * @param stream {ACL_VSTREAM*} 受监控的流, 当该流有完整的一行数据、出错
 *  或读超时时将回调用户的注册函数.
 * @return {ACL_ASTREAM*} 异步通信流句柄
 */
ACL_API ACL_ASTREAM *acl_aio_open(ACL_AIO *aio, ACL_VSTREAM *stream);

/**
 * 异步IO完成后关闭流，否则进行异步关闭动作，即需要等读写都完成时才关闭流
 * @param astream {ACL_ASTREAM*} 异步数据流
 */
ACL_API void acl_aio_iocp_close(ACL_ASTREAM *astream);

/**
 * 取消异步IO过程，该功能主要是为了将异步IO流转换为同步IO流而写
 * @param astream {ACL_ASTREAM*} 异步IO流
 * @return {ACL_VSTREAM*} 流句柄
 */
ACL_API ACL_VSTREAM *acl_aio_cancel(ACL_ASTREAM *astream);

/**
 * 获得监听描述符每次接收客户端连接的最大个数
 * @param astream {ACL_ASTREAM *} 监听描述符流
 * @return {int} 每次接收连接的最大个数
 * @return {int} 监听描述符在每次接收过程中可以循环接收的最大连接
 *  个数，此值最小为1
 */
ACL_API int acl_aio_get_accept_max(ACL_ASTREAM *astream);

/**
 * 设置监听描述符每次接收客户端连接的最大个数
 * @param astream {ACL_ASTREAM *} 监听描述符流
 * @param accept_max {int} 监听描述符在每次接收过程中可以循环接收的最大连接
 *  个数，此值最小为1
 */
ACL_API void acl_aio_set_accept_max(ACL_ASTREAM *astream, int accept_max);

/**
 * 添加附加读回调函数
 * @param astream {ACL_ASTREAM*} 异步流，不能为空
 * @param callback {ACL_AIO_READ_FN} 回调函数，不能为空
 * @param ctx {void*} callback 回调函数的回调参数，可以为空
 */
ACL_API void acl_aio_add_read_hook(ACL_ASTREAM *astream,
	ACL_AIO_READ_FN callback, void *ctx);

/**
 * 添加附加写回调函数
 * @param astream {ACL_ASTREAM*} 异步流，不能为空
 * @param callback {ACL_AIO_READ_FN} 回调函数，不能为空
 * @param ctx {void*} callback 回调函数的回调参数，可以为空
 */
ACL_API void acl_aio_add_write_hook(ACL_ASTREAM *astream,
	ACL_AIO_WRITE_FN callback, void *ctx);

/**
 * 添加附加关闭回调函数
 * @param astream {ACL_ASTREAM*} 异步流，不能为空
 * @param callback {ACL_AIO_READ_FN} 回调函数，不能为空
 * @param ctx {void*} callback 回调函数的回调参数，可以为空
 */
ACL_API void acl_aio_add_close_hook(ACL_ASTREAM *astream,
	ACL_AIO_CLOSE_FN callback, void *ctx);

/**
 * 添加附加超时回调函数
 * @param astream {ACL_ASTREAM*} 异步流，不能为空
 * @param callback {ACL_AIO_READ_FN} 回调函数，不能为空
 * @param ctx {void*} callback 回调函数的回调参数，可以为空
 */
ACL_API void acl_aio_add_timeo_hook(ACL_ASTREAM *astream,
	ACL_AIO_TIMEO_FN callback, void *ctx);

/**
 * 添加附加连接成功回调函数
 * @param astream {ACL_ASTREAM*} 异步流，不能为空
 * @param callback {ACL_AIO_READ_FN} 回调函数，不能为空
 * @param ctx {void*} callback 回调函数的回调参数，可以为空
 */
ACL_API void acl_aio_add_connect_hook(ACL_ASTREAM *astream,
	ACL_AIO_CONNECT_FN callback, void *ctx);

/**
 * 删除附加读回调函数
 * @param astream {ACL_ASTREAM*} 异步流，不能为空
 * @param callback {ACL_AIO_READ_FN} 回调函数，不能为空
 * @param ctx {void*} callback 回调函数的回调参数，可以为空
 */
ACL_API void acl_aio_del_read_hook(ACL_ASTREAM *astream,
	ACL_AIO_READ_FN callback, void *ctx);

/**
 * 删除附加写回调函数
 * @param astream {ACL_ASTREAM*} 异步流，不能为空
 * @param callback {ACL_AIO_READ_FN} 回调函数，不能为空
 * @param ctx {void*} callback 回调函数的回调参数，可以为空
 */
ACL_API void acl_aio_del_write_hook(ACL_ASTREAM *astream,
	ACL_AIO_WRITE_FN callback, void *ctx);

/**
 * 删除附加关闭回调函数
 * @param astream {ACL_ASTREAM*} 异步流，不能为空
 * @param callback {ACL_AIO_READ_FN} 回调函数，不能为空
 * @param ctx {void*} callback 回调函数的回调参数，可以为空
 */
ACL_API void acl_aio_del_close_hook(ACL_ASTREAM *astream,
	ACL_AIO_CLOSE_FN callback, void *ctx);

/**
 * 删除附加超时回调函数
 * @param astream {ACL_ASTREAM*} 异步流，不能为空
 * @param callback {ACL_AIO_READ_FN} 回调函数，不能为空
 * @param ctx {void*} callback 回调函数的回调参数，可以为空
 */
ACL_API void acl_aio_del_timeo_hook(ACL_ASTREAM *astream,
	ACL_AIO_TIMEO_FN callback, void *ctx);

/**
 * 删除附加连接成功回调函数
 * @param astream {ACL_ASTREAM*} 异步流，不能为空
 * @param callback {ACL_AIO_READ_FN} 回调函数，不能为空
 * @param ctx {void*} callback 回调函数的回调参数，可以为空
 */
ACL_API void acl_aio_del_connect_hook(ACL_ASTREAM *astream,
	ACL_AIO_CONNECT_FN callback, void *ctx);

/**
 * 清除所有的附加读回调函数
 * @param astream {ACL_ASTREAM*} 异步流，不能为空
 */
ACL_API void acl_aio_clean_read_hooks(ACL_ASTREAM *astream);

/**
 * 清除所有的附加写回调函数
 * @param astream {ACL_ASTREAM*} 异步流，不能为空
 */
ACL_API void acl_aio_clean_write_hooks(ACL_ASTREAM *astream);

/**
 * 清除所有的附加关闭回调函数
 * @param astream {ACL_ASTREAM*} 异步流，不能为空
 */
ACL_API void acl_aio_clean_close_hooks(ACL_ASTREAM *astream);

/**
 * 清除所有的附加超时回调函数
 * @param astream {ACL_ASTREAM*} 异步流，不能为空
 */
ACL_API void acl_aio_clean_timeo_hooks(ACL_ASTREAM *astream);

/**
* 清除所有的附加连接成功回调函数
* @param astream {ACL_ASTREAM*} 异步流，不能为空
*/
ACL_API void acl_aio_clean_connect_hooks(ACL_ASTREAM *astream);

/**
 * 清除所有的附加回调函数
 * @param astream {ACL_ASTREAM*} 异步流，不能为空
 */
ACL_API void acl_aio_clean_hooks(ACL_ASTREAM *astream);

/**
 * 设置异步流的属性
 * @param astream {ACL_ASTREAM*} 异步流对象
 * @param name {int} 第一个控制参数
 * @param ... 变参列表，格式为：ACL_AIO_CTL_XXX, xxx, 最后一个控制参数
 *  为 ACL_AIO_CTL_END
 */
ACL_API void acl_aio_ctl(ACL_ASTREAM *astream, int name, ...);
#define ACL_AIO_CTL_END                 0   /**< 控制结束标志 */
#define ACL_AIO_CTL_ACCEPT_FN           1   /**< 设置接收连接后回调函数 */
#define ACL_AIO_CTL_LISTEN_FN           2   /**< 设置有连接到达时回调函数 */
#define ACL_AIO_CTL_CTX                 3   /**< 设置应用的参数 */
#define ACL_AIO_CTL_TIMEOUT             4   /**< 设置超时时间 */
#define	ACL_AIO_CTL_LINE_LENGTH         5   /**< 设置所读行数据的最大长长度 */
#define ACL_AIO_CTL_STREAM              10  /**< 设置ACL_VSTREAM流指针 */
#define ACL_AIO_CTL_READ_NESTED         11  /**< 设置最大读嵌套层数 */
#define ACL_AIO_CTL_WRITE_NESTED        12  /**< 设置最大写嵌套层数 */
#define ACL_AIO_CTL_KEEP_READ           13  /**< 设置是否连续读标志 */
#define	ACL_AIO_CTL_READ_HOOK_ADD       14  /**< 添加附加读回调函数 */
#define	ACL_AIO_CTL_READ_HOOK_DEL       15  /**< 删除附加读回调函数 */
#define	ACL_AIO_CTL_WRITE_HOOK_ADD      16  /**< 添加附加写回调函数 */
#define	ACL_AIO_CTL_WRITE_HOOK_DEL      17  /**< 删除附加写回调函数 */
#define	ACL_AIO_CTL_CLOSE_HOOK_ADD      18  /**< 添加附加关闭回调函数 */
#define	ACL_AIO_CTL_CLOSE_HOOK_DEL      19  /**< 删除附加关闭回调函数 */
#define	ACL_AIO_CTL_TIMEO_HOOK_ADD      20  /**< 添加附加超时回调函数 */
#define	ACL_AIO_CTL_TIMEO_HOOK_DEL      21  /**< 删除附加超时回调函数 */
#define	ACL_AIO_CTL_CONNECT_HOOK_ADD    22  /**< 添加附加连接回调函数 */
#define	ACL_AIO_CTL_CONNECT_HOOK_DEL    23  /**< 删除附加连接回调函数 */

/**
 * 从异步流中提取 ACL_VSTREAM 流
 * @param astream {ACL_ASTREAM*} 异步IO流
 * @return {ACL_VSTREAM*} 通信流指针
 */
ACL_API ACL_VSTREAM *acl_aio_vstream(ACL_ASTREAM *astream);

/*---------------------------- 异步读操作接口 --------------------------------*/

/**
 * 异步从流中读取一行数据, 当成功读取一行数据、出错、读超时时将回调用户的
 * 注册函数: notify_fn
 * @param astream {ACL_ASTREAM*} 受监控的流, 当该流有完整的一行数据、出错
 *  或读超时时将回调用户的注册函数.
 * 注: 读操作发生在异步框架内.
 *     当通过 acl_aio_stream_set_line_length 设置了行最大长度限制，则当接收的
 *     数据行过大时，为避免缓冲区溢出，该函数的处理过程将会在缓冲区达到该长度
 *     限制时被触发，直接将数据交由使用者注册的回调过程
 */
ACL_API void acl_aio_gets(ACL_ASTREAM *astream);

/**
 * 异步从流中读取一行数据, 当成功读取一行数据、出错、读超时时将回调用户的
 * 注册函数: notify_fn, 与 acl_aio_gets 功能类似, 但唯一的区别是返回的数据
 * data 中不包含 "\r\n" 或 "\n", 当读到一个空行时, 则 dlen == 0.
 * @param astream {ACL_ASTREAM*} 受监控的流, 当该流有完整的一行数据、出错
 *  或读超时时将回调用户的注册函数.
 * 注: 读操作发生在异步框架内.
 *     数据行过大时，为避免缓冲区溢出，该函数的处理过程将会在缓冲区达到该长度
 *     限制时被触发，直接将数据交由使用者注册的回调过程
 */
ACL_API void acl_aio_gets_nonl(ACL_ASTREAM *astream);

/**
 * 异步从流中读取数据, 读取的数据格式及长度没有特殊要求.
 * @param astream {ACL_ASTREAM*} 处于读监控的流. 当该流出错、超时或已经读取了一定
 *  长度的数据时将触发事件通知过程
 * 注: 读操作发生在异步框架内.
 */
ACL_API void acl_aio_read(ACL_ASTREAM *astream);

/**
 * 异步从流中读取要求长度的数据, 当流出错、超时或读到了所要求的数据长度时将
 * 触发事件通知过程
 * @param astream {ACL_ASTREAM*} 处于读监控的流. 当该流出错、超时或已经读取了所
 *  要求长度的数据时将触发事件通知过程
 * @param count {int} 所要求的数据的长度, 必须大于 0.
 * 注: 读操作发生在异步框架内.
 */
ACL_API void acl_aio_readn(ACL_ASTREAM *astream, int count);

/**
 * 尝试性读取一行数据
 * @param astream {ACL_ASTREM*} 异步流对象
 * @return {ACL_VSTRING*} 若读得完整一行则返回非空对象，用户用完此 ACL_VSTRING
 *  数据后应调用 ACL_VSTRING_RESET(s) 清空缓冲区; 若未读得完整行则返回空
 */
ACL_API ACL_VSTRING *acl_aio_gets_peek(ACL_ASTREAM *astream);

/**
 * 尝试性读取一行数据(不包含 \n 或 \r\n)
 * @param astream {ACL_ASTREM*} 异步流对象
 * @return {ACL_VSTRING*} 若读得完整一行则返回非空对象，用户用完此 ACL_VSTRING
 *  数据后应调用 ACL_VSTRING_RESET(s) 清空缓冲区, 另外如果读到一个空行，则返回的
 *  ACL_VSTRING 的缓冲区的数据长度(ACL_VSTRING_LEN 获得此值) 应为 0;
 *  若未读得完整行则返回空
 */
ACL_API ACL_VSTRING *acl_aio_gets_nonl_peek(ACL_ASTREAM *astream);

/**
 * 尝试性从异步流中读取数据，如果有数据则返回没有则返回空
 * @param astream {ACL_ASTREM*} 异步流对象
 * @param count {int*} 函数返回后将存放本次读到的数据长度，返回值永远 >= 0
 * @return {ACL_VSTRING*} 若读到了数据则返回的缓冲区非空(使用者用完此缓冲区后
 *  需要调用 ACL_VSTRING_RESET(s) 清空此缓冲区), 否则返回空
 */
ACL_API ACL_VSTRING *acl_aio_read_peek(ACL_ASTREAM *astream, int *count);

/**
 * 尝试性从异步流中读给定长度的数据，如果读到的数据满足要求则返回缓冲区
 * @param astream {ACL_ASTREM*} 异步流对象
 * @param count {int*} 要求读到的数据长度，函数返回后将存放本次读到的字节数，
 *  存放的值永远 >= 0
 * @return {ACL_VSTRING*} 若读到规定长度则返回非空缓冲区(使用者用完此缓冲区后
 *  需要调用 ACL_VSTRING_RESET(s) 清空此缓冲区), 否则返回空
 */
ACL_API ACL_VSTRING *acl_aio_readn_peek(ACL_ASTREAM *astream, int *count);

/**
 * 设置异步流为读监听状态，当该流可读时则调用用户的回调函数
 * @param astream {ACL_ASTREM*} 异步流对象
 * @param can_read_fn {ACL_AIO_NOTIFY_FN} 用户回调函数
 * @param context {void*} can_read_fn 的参数之一
 */
ACL_API void acl_aio_enable_read(ACL_ASTREAM *astream,
	ACL_AIO_NOTIFY_FN can_read_fn, void *context);

/**
 * 检测异步流有多少数据可读
 * @param astream {ACL_ASTREM*} 异步流对象
 * @return {int} ACL_VSTREAM_EOF 表示出错，应该关闭该流; 0 表示无数据可读;
 *  > 0 表示有数据可读
 */
ACL_API int acl_aio_can_read(ACL_ASTREAM *astream);

/**
 * 停止对一个数据流进行IO读操作
 * @param astream {ACL_ASTREAM*} 异步数据流
 */
ACL_API void acl_aio_disable_read(ACL_ASTREAM *astream);

/**
 * 判断流是否在异步事件的读监听集合中
 * @param astream {ACL_ASTREAM*} 异步数据流
 * @return {int} 0: 否，!= 0: 是
 */
ACL_API int acl_aio_isrset(ACL_ASTREAM *astream);

/**
 * 设置读一行数据时每行数据的最大长度限制，这样的目的主要是为了防止对方发送的
 * 一行数据过长，造成本地接收缓冲区内存溢出
 * @param astream {ACL_ASTREAM*} 异步数据流
 * @param len {int} 当该值 > 0 时将会限制按行读的数据长度
 */
ACL_API void acl_aio_stream_set_line_length(ACL_ASTREAM *astream, int len);

/**
 * 获得所设置的流按行读数据时的最大长度限制
 * @param astream {ACL_ASTREAM*} 异步数据流
 * @return {int}
 */
ACL_API int acl_aio_stream_get_line_length(ACL_ASTREAM *astream);

/**
 * 单独设置异步流的连续读标记，缺省情况下自动继承 ACL_AIO 中的 keep_read
 * 标记(其默认情况下是连续读)
 * @param astream {ACL_ASTREAM*} 异步数据流
 * @param onoff {int} 0 表示关闭连续读功能，非0表示打开连续读功能
 */
ACL_API void acl_aio_stream_set_keep_read(ACL_ASTREAM *astream, int onoff);

/**
 * 获得异步流是否是设置了连续读标记
 * @return {int} 0 表示关闭了连续读功能，非0表示打开了连续读功能
 */
ACL_API int acl_aio_stream_get_keep_read(ACL_ASTREAM *astream);

/*---------------------------- 异步写操作接口 --------------------------------*/

/**
 * 异步向流中写数据, 当流出错、写超时或写成功时将触发事件通知过程
 * @param astream {ACL_ASTREAM*} 处于写监控的流.
 * @param data {const char*} 所写数据的内存开始指针位置
 * @param dlen {int} data 中数据长度
 */
ACL_API void acl_aio_writen(ACL_ASTREAM *astream, const char *data, int dlen);

/**
 * 异步向流中写数据, 当流出错、写超时或写成功时将触发事件通知过程，类似系统的
 * writev
 * @param astream {ACL_ASTREAM*} 处于写监控的流.
 * @param vector {const struct iovec*} 数据集合数组
 * @param count {int} vector 数组的长度
 */
ACL_API void acl_aio_writev(ACL_ASTREAM *astream,
		const struct iovec *vector, int count);

/**
 * 以格式方式异步向流中写数据, 当流出错、写超时或写成功时将触发事件通知过程
 * @param astream {ACL_ASTREAM*} 处于写监控的流
 * @param fmt {const char*} 格式字符串
 * @param ap {va_list} 格式字符串的参数列表
 */
ACL_API void acl_aio_vfprintf(ACL_ASTREAM *astream, const char *fmt, va_list ap);

/**
 * 以格式方式异步向流中写数据, 当流出错、写超时或写成功时将触发事件通知过程
 * @param astream {ACL_ASTREAM*} 处于写监控的流
 * @param fmt {const char*} 格式字符串
 * @param ... 变参参数表
 */
ACL_API void ACL_PRINTF(2, 3) acl_aio_fprintf(ACL_ASTREAM *astream, const char *fmt, ...);

/**
 * 设置异步流为写监听状态，当该流可写时则调用用户的回调函数
 * @param astream {ACL_ASTREM*} 异步流对象
 * @param can_write_fn {ACL_AIO_NOTIFY_FN} 用户回调函数
 * @param context {void*} can_write_fn 的参数之一
 */
ACL_API void acl_aio_enable_write(ACL_ASTREAM *astream,
	ACL_AIO_NOTIFY_FN can_write_fn, void *context);

/**
 * 停止对一个数据流进行IO写操作
 * @param astream {ACL_ASTREAM*} 异步数据流
 */
ACL_API void acl_aio_disable_write(ACL_ASTREAM *astream);

/**
 * 判断流是否在异步事件的写监听集合中
 * @param astream {ACL_ASTREAM*} 异步数据流
 * @return {int} 0: 否，!= 0: 是
 */
ACL_API int acl_aio_iswset(ACL_ASTREAM *astream);

/*---------------------------- 异步监听操作接口 ------------------------------*/

/**
 * 异步接收一个客户端连接流, 并将该客户端流回传给用户
 * @param astream {ACL_ASTREAM*} 处于监听状态的流
 */
ACL_API void acl_aio_accept(ACL_ASTREAM *astream);

/**
 * 异步监听, 当监听流上出错、超时或有新连接到达时将触发监听事件通知过程, 当有
 * 新连接时用户需在自己的注册函数里 accept() 该新连接.
 * @param astream {ACL_ASTREAM*} 处于监听状态的流
 */
ACL_API void acl_aio_listen(ACL_ASTREAM *astream);

/*---------------------------- 异步连接操作接口 ------------------------------*/

/**
 * 异步连接一个远程服务器, 当连接流出错、超时或连接成功时将触发事件通知过程.
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 * @param addr {const char*} 远程服务器地址, 格式: ip:port, 如: 192.168.0.1:80
 * @param timeout {int} 连接超时的时间值，单位为秒
 * @return {ACL_ASTREAM*} 创建异步连接过程是否成功
 */
ACL_API ACL_ASTREAM *acl_aio_connect(ACL_AIO *aio, const char *addr, int timeout);

/**
 * 异步连接一个远程服务器，给定的地址可以是域名，以区别于 acl_aio_connect 函数，
 * 使用本函数的首要条件是必须通过 acl_aio_set_dns 设置的域名服务器的地址
 * @param aio {ACL_AIO*} 异步框架引擎句柄
 * @param addr {const char*} 服务器地址，格式：domain:port，如：www.sina.com:80
 * @param timeout {int} 连接超时的时间值，单位为秒
 * @param callback {ACL_AIO_CONNECT_ADDR_FN}
 * @param context {void*} 传递给 callback 回调函数的参数
 * @return {int} 返回 0 表示开始异步域名解析及异步连接过程，返回 < 0 表示传入的
 *  参数有误或在创建 ACL_AIO 句柄后没有通过 acl_aio_set_dns 函数设置域名服务器
 */
ACL_API int acl_aio_connect_addr(ACL_AIO *aio, const char *addr, int timeout,
		ACL_AIO_CONNECT_ADDR_FN callback, void *context);

/*---------------------------- 其它通用异步操作接口 --------------------------*/

/**
 * 停止对一个数据流进行IO读写操作
 * @param astream {ACL_ASTREAM*} 异步数据流
 */
ACL_API void acl_aio_disable_readwrite(ACL_ASTREAM *astream);

/**
 * 判断流是否在异步事件的读或写监听集合中
 * @param astream {ACL_ASTREAM*} 异步数据流
 * @return {int} 0: 否，!= 0: 是
 */
ACL_API int acl_aio_isset(ACL_ASTREAM *astream);

/**
 * 获得当前异步流的引用计数值
 * @param astream {ACL_ASTREAM*} 异步数据流
 * @return {int} >=0，异步流的引用计数值
 */
ACL_API int acl_aio_refer_value(ACL_ASTREAM * astream);

/**
 * 将异步流的引用计数值加1
 * @param astream {ACL_ASTREAM*} 异步数据流
 */
ACL_API void acl_aio_refer(ACL_ASTREAM *astream);

/**
 * 将异步流的引用计数值减1
 * @param astream {ACL_ASTREAM*} 异步数据流
 */
ACL_API void acl_aio_unrefer(ACL_ASTREAM *astream);

/**
 * 添加一个定时器任务, 该函数仅是 acl_event_request_timer 的简单封装
 * @param aio {ACL_AIO*} 异步通信引擎句柄
 * @param timer_fn {ACL_EVENT_NOTIFY_TIME} 定时器任务回调函数.
 * @param context {void*} timer_fn 的参数之一.
 * @param idle_limit {acl_int64} 启动定时器函数的时间，单位为微秒.
 * @param keep {int} 是否重复定时器任务
 * @return {acl_int64} 剩余的时间, 单位为微秒.
 */
ACL_API acl_int64 acl_aio_request_timer(ACL_AIO *aio,
		ACL_EVENT_NOTIFY_TIME timer_fn, void *context,
		acl_int64 idle_limit, int keep);

/**
 * 取消某个定时器任务, 该函数仅是 acl_event_cancel_timer 的简单封装.
 * @param aio {ACL_AIO*} 异步通信引擎句柄
 * @param timer_fn {ACL_EVENT_NOTIFY_TIME} 定时器任务回调函数.
 * @param context {void*} timer_fn 的参数之一.
 * @return {acl_int64} 剩余的时间, 单位为微秒.
 */
ACL_API acl_int64 acl_aio_cancel_timer(ACL_AIO *aio,
		ACL_EVENT_NOTIFY_TIME timer_fn, void *context);

/**
 * 设置是否需要循环启用通过 acl_aio_request_timer 设置的定时器任务
 * @param aio {ACL_AIO*} 异步通信引擎句柄
 * @param timer_fn {ACL_EVENT_NOTIFY_TIME} 定时器任务回调函数.
 * @param context {void*} timer_fn 的参数之一.
 * @param onoff {int} 是否重复定时器任务
 */
ACL_API void acl_aio_keep_timer(ACL_AIO *aio, ACL_EVENT_NOTIFY_TIME timer_fn,
		void *context, int onoff);

/**
 * 判断所设置的定时器都处于重复使用状态
 * @param aio {ACL_AIO*} 异步通信引擎句柄
 * @param timer_fn {ACL_EVENT_NOTIFY_TIME} 定时器任务回调函数.
 * @param context {void*} timer_fn 的参数之一.
 * @return {int} !0 表示所设置的定时器都处于重复使用状态
 */
ACL_API int acl_aio_timer_ifkeep(ACL_AIO *aio, ACL_EVENT_NOTIFY_TIME timer_fn,
		void *context);

#ifdef	__cplusplus
}
#endif

#endif
