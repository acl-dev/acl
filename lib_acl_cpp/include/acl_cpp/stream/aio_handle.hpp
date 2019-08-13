#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"

struct ACL_AIO;
struct ACL_EVENT;

namespace acl
{

// 事件引擎类型
typedef enum
{
	ENGINE_SELECT,  // select 模式(支持所有平台)
	ENGINE_POLL,    // poll 模式(仅 UNIX 平台)
	ENGINE_KERNEL,  // kernel 模式(win32: iocp, Linux: epoll, FreeBsd: kqueue, Solaris: devpoll
	ENGINE_WINMSG   // win32 GUI 消息模式
} aio_handle_type;

/**
 * 非阻塞IO的事件引擎类，该类封装了系统的 select/poll/epoll/kqueue/devpoll/iocp,
 */

class aio_timer_callback;
class aio_delay_free;
class aio_timer_delay_free;

class ACL_CPP_API aio_handle : private noncopyable
{
public:
	/**
	 * 构造函数，会自动创建IO事件引擎，并且在析构函数中会自动释放
	 * @param engine_type {aio_handle_type} 所采用的引擎类型
	 *  ENGINE_SELECT: select 方式，支持 win32/unix 平台
	 *  ENGINE_POLL: poll 方式，支持 unix 平台
	 *  ENGINE_KERNEL: 自动根据各个系统平台所支持的高效内核引擎进行设置
	 *  ENGINE_WINMSG: win32 界面消息方式，支持 win32 平台
	 * @param nMsg {unsigned int} 若 engine_type 为 ENGINE_WINMSG，当该值
	 *  大于 0 时，该异步句柄便与该消息绑定，否则与缺省消息绑定；
	 *  当 engine_type 为非 ENGINE_WINMSG 时，该值对其它异步句柄不起作用
	 *  
	 */
	aio_handle(aio_handle_type engine_type = ENGINE_SELECT,
		unsigned int nMsg = 0);

	/**
	 * 构造函数，调用者将 ACL_AIO 句柄传进，而在类的析构函数中并不会
	 * 自动释放该 ACL_AIO 句柄
	 * @param handle {ACL_AIO*} ACL_AIO 句柄
	 */
	aio_handle(ACL_AIO* handle);

	virtual ~aio_handle(void);

	/**
	 * 针对异步读流，设置是否是连续读，该配置项将会被所有的基于
	 * 该异步引擎句柄的异步读流所继承，一般 aio_handle 类对象在缺省
	 * 情况下是连续读的
	 * @param onoff {bool} 设置是否是连续读
	 */
	void keep_read(bool onoff);

	/**
	 * 获得异步引擎句柄是否设置了持续读数据的功能
	 * @return {bool}
	 */
	bool keep_read(void) const;

	/**
	 * 设置定时器
	 * @param callback {aio_timer_callback*} 定时器回调函数类对象
	 * @param delay {int64} 定时器时间间隔(微秒)
	 * @param id {unsigned int} 定时器某个任务的 ID 号
	 * @return {int64} 定时器生效时间(从1970.1.1以来的微秒数)
	 */
#if defined(_WIN32) || defined(_WIN64)
	__int64 set_timer(aio_timer_callback* callback,
		__int64 delay, unsigned int id = 0);
#else
	long long int set_timer(aio_timer_callback* callback,
		long long int delay, unsigned int id = 0);
#endif

	/**
	 * 删除定时器的所有定时任务事件
	 * @param callback {aio_timer_callback*} 定时器回调函数类对象
	 * @return {time_t} 定时器生效时间(从1970.1.1以来的微秒数)
	 */
#if defined(_WIN32) || defined(_WIN64)
	__int64 del_timer(aio_timer_callback* callback);
#else
	long long int del_timer(aio_timer_callback* callback);
#endif

	/**
	 * 删除定时器中某个指定 ID 号的定时任务
	 * @param callback {aio_timer_callback*} 定时器回调函数类对象
	 * @param id {unsigned int} 定时器某个任务的 ID 号
	 * @return {time_t} 定时器生效时间(从1970.1.1以来的微秒数)
	 */
#if defined(_WIN32) || defined(_WIN64)
	__int64 del_timer(aio_timer_callback* callback, unsigned int id);
#else
	long long del_timer(aio_timer_callback* callback, unsigned int id);
#endif

	/**
	 * 当定时器处于锁定状态时，用户因为无法释放该定时器而造成内存泄露，
	 * 通过此函数，可以将处于锁定状态的定时器当处于未锁定状态时被事件
	 * 引擎延期释放(调用 aio_delay_free::destroy())，从而可以避免
	 * 内存泄露问题
	 * @param callback {aio_delay_free*}
	 */
	void delay_free(aio_delay_free* callback);

	/**
	 * 获得 ACL_AIO 句柄
	 * @return {ACL_AIO*}
	 */
	ACL_AIO* get_handle(void) const;

	/**
	 * 获得异步引擎的类型
	 * @return {aio_handle_type}
	 */
	aio_handle_type get_engine_type(void) const;

	/**
	 * 获得当前处于监控的异步流的数量
	 * @return {int}
	 */
	int length(void) const;

	/**
	 * 检查所有异步流的状态，并触发准备的异步流的处理过程
	 * @return {bool} 是否应中止异步引擎
	 */
	bool check(void);

	/**
	 * 获得本次事件循环被触发的事件次数
	 * @return {int}
	 */
	int last_nready(void) const;

	/**
	 * 通知异步流引擎中止
	 */
	void stop(void);

	/**
	 * 重置异步引擎的内部状态
	 */
	void reset(void);

	/**
	 * 设置 DNS 服务器地址列表，格式：ip1:port1;ip2:port2...
	 * @param addrs {const char*} DNS 服务器地址列表，如：8.8.8.8:53;1.1.1.1:53
	 * @param timeout {int} DNS 查询超时时间（秒）
	 *  注：set_dns 和 dns_add 执行相同的功能
	 */
	void set_dns(const char* addrs, int timeout);
	void dns_add(const char* addrs, int timeout);

	/**
	 * 删除指定的 DNS 服务器地址列表，格式：ip1:port1;ip2:port2...
	 * @param addrs {const char*} DNS 服务器地址列表
	 */
	void dns_del(const char* addrs);

	/**
	 * 清除掉所设置的所有 DNS 服务器列表
	 */
	void dns_clear(void);

	/**
	 * DNS 服务器列表数量
	 * @return {size_t}
	 */
	size_t dns_size(void) const;

	/**
	 * 判断 DNS 服务器列表是否为空
	 * @return {bool}
	 */
	bool dns_empty(void) const;
	
	/**
	 * 获得 DNS 服务器地址列表
	 * @param out {std::vector<std::pair<acl::string, unsigned short> >&}
	 */
	void dns_list(std::vector<std::pair<string, unsigned short> >& out);

public:
	/**
	 * 设置异步引擎循环的等待时间中的秒级部分
	 * @param n {int} 设置用 select/poll/epoll/kqueue/devpoll
	 *  时的秒级等待时间
	 */
	void set_delay_sec(int n);

	/**
	 * 设置异步引擎循环的等待时间中的微秒级部分
	 * @param n {int} 设置用 select/poll/epoll/kqueue/devpoll
	 *  时的微秒级等待时间
	 */
	void set_delay_usec(int n);

	/**
	 * 设置事件循环过程中定时检查所有描述字状态的时间间隔，
	 * 内部缺省值 100 ms
	 */
	void set_check_inter(int n);

	/**
	 * 设置异步流的读缓存区大小
	 * @param n {int} 读缓冲区大小
	 */
	void set_rbuf_size(int n);

protected:
	friend class aio_stream;

	/**
	 * 异步流个数加 1
	 */
	void increase(void);

	/**
	 * 当异步流个数加 1 时的回调虚函数
	 */
	virtual void on_increase(void) {}

	/**
	 * 异步流个数减 1
	 */
	void decrease(void);

	/**
	 * 当异步流个数减 1 时的回调虚函数
	 */
	virtual void on_decrease(void) {}

private:
	ACL_AIO* aio_;
	bool inner_alloc_;
	bool stop_;
	int  nstream_;
	aio_handle_type engine_type_;
	aio_timer_delay_free* delay_free_timer_;

	void destroy_timer(aio_timer_callback* callback);
	static void on_timer_callback(int event_type, ACL_EVENT*,
		aio_timer_callback *callback);
};

} // namespace acl
