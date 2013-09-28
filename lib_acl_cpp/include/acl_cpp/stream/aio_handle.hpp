#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/noncopyable.hpp"
#include <list>
#include <set>
//#include "string.hpp"

struct ACL_AIO;
struct ACL_EVENT;

namespace acl
{

/**
 * 需要被延迟释放的类继承此类后，可以调用 aio_handle:delay_free 来到达
 * 延迟销毁的目的，避免了在递归过程中被立即释放时的对象被提前释放的问题
 */
class ACL_CPP_API aio_delay_free
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

class aio_timer_task;
class aio_handle;

/**
 * 定时器的回调类
 */
class ACL_CPP_API aio_timer_callback : public aio_delay_free
{
public:
	/**
	 * 构造函数
	 * @param keep {bool} 该定时器是否允许自动重启
	 */
	aio_timer_callback(bool keep = false);
	virtual ~aio_timer_callback(void);

	/**
	 * 当定时器里的任务数为空时的回调函数，
	 * 子类可以在其中释放，一旦该函数被调用，
	 * 则意味着该定时器及其中的所有定时任务都从
	 * 定时器集合中被删除
	 * 该函数被触发的条件有三个：
	 * 1) 定时器所有的任务数为 0 时(如，
	 *    del_timer(aio_timer_callback*, unsigned int) 被
	 *    调用且任务数为 0 时)
	 * 2) 当 aio_handle 没有设置重复定时器且该定时器中
	 *    有一个定时任务被触发后
	 * 3) 当 del_timer(aio_timer_callback*) 被调用后
	 */
	virtual void destroy(void) {}

	/**
	 * 定时器里的任务是否为空
	 * @return {bool}
	 */
	bool empty(void) const;

	/**
	 * 定时器里的任务个数
	 * @return {size_t}
	 */
	size_t length(void) const;

	/**
	 * 该定时器是否是自动重启的
	 * @param on {bool}
	 */
	void keep_timer(bool on);

	/**
	 * 判断该定时器是否是自动重启的
	 * @return {bool}
	 */
	bool keep_timer(void) const;

	/**
	 * 清空定时器里的定时任务
	 * @return {int} 被清除的定时任务的个数
	 */
	int clear(void);

protected:
	friend class aio_handle;

	/**
	 * 子类必须实现此回调函数，注：子类或调用者禁止在
	 * timer_callback 内部调用 aio_timer_callback 的析构
	 * 函数，否则将会酿成大祸
	 * @param id {unsigned int} 对应某个任务的 ID 号
	 */
	virtual void timer_callback(unsigned int id) = 0;

	/************************************************************************/
	/*        子类可以调用如下函数添加一些新的定时器任务 ID 号              */
	/************************************************************************/
#ifdef WIN32
	__int64 present_;

	/**
	 * 针对本定时器增加新的任务ID号，这样便可以通过一个定时器启动
	 * 多个定时任务
	 * @param id {unsigned int} 定时器定时任务ID号
	 * @param delay {__int64} 每隔多久自动触发该定时器，同时将对应的定时器定时
	 *  任务ID号传回(微秒级)
	 * @return {__int64} 距离本定时器的第一个将会触发的定时任务ID还多久(微秒级)
	 */
	__int64 set_task(unsigned int id, __int64 delay);

	/**
	 * 删除定时器中某个消息ID对应的定时任务
	 * @param {unsigned int} 定时任务ID
	 * @return {__int64} 距离本定时器的第一个将会触发的定时任务ID还多久(微秒级)
	 */
	__int64 del_task(unsigned int id);
#else
	long long int present_;
	long long int set_task(unsigned int id, long long int delay);
	long long int del_task(unsigned int id);
#endif

	/**
	 * 设置当前定时器的时间截
	 */
	void set_time(void);
private:
	aio_handle* handle_;
	size_t length_;
	std::list<aio_timer_task*> tasks_;
	bool keep_;  // 该定时器是否允许自动重启
	bool destroy_on_unlock_;  // 解锁后是否 destroy
#ifdef WIN32
	__int64 set_task(aio_timer_task* task);
	__int64 trigger(void);
#else
	long long int set_task(aio_timer_task* task);
	long long int trigger(void);
#endif
};

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
	aio_handle(aio_handle_type engine_type = ENGINE_SELECT, unsigned int nMsg = 0);

	/**
	 * 构造函数，调用者将 ACL_AIO 句柄传进，而在类的析构函数中并不会
	 * 自动释放该 ACL_AIO 句柄
	 * @param handle {ACL_AIO*} ACL_AIO 句柄
	 */
	aio_handle(ACL_AIO* handle);

	virtual ~aio_handle();

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
	bool keep_read() const;

	/**
	 * 设置定时器
	 * @param callback {aio_timer_callback*} 定时器回调函数类对象
	 * @param delay {int64} 定时器时间间隔(微秒)
	 * @param id {unsigned int} 定时器某个任务的 ID 号
	 * @return {int64} 定时器生效时间(从1970.1.1以来的微秒数)
	 */
#ifdef WIN32
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
#ifdef WIN32
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
#ifdef WIN32
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
	int length() const;

	/**
	 * 检查所有异步流的状态，并触发准备的异步流的处理过程
	 * @return {bool} 是否应中止异步引擎
	 */
	bool check();

	/**
	 * 通知异步流引擎中止
	 */
	void stop();

	/**
	 * 重置异步引擎的内部状态
	 */
	void reset();

protected:
	friend class aio_stream;

	/**
	 * 异步流个数加 1
	 */
	void increase();

	/**
	 * 当异步流个数加 1 时的回调虚函数
	 */
	virtual void on_increase() {}

	/**
	 * 异步流个数减 1
	 */
	void decrease();

	/**
	 * 当异步流个数减 1 时的回调虚函数
	 */
	virtual void on_decrease() {}

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
