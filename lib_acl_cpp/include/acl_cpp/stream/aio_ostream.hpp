#pragma once
#include "../acl_cpp_define.hpp"
#include <stdarg.h>
#include "../stdlib/string.hpp"
#include "aio_handle.hpp"
#include "aio_timer_callback.hpp"
#include "aio_stream.hpp"

namespace acl
{

class aio_ostream;

/**
 * 延迟异步写数据类，基类为 aio_timer_callback (see aio_handle.hpp)，
 * 所谓延迟异步写，就是把异步写流(aio_ostream)放在定时器中，将该异
 * 步流的异步写操作解除绑定(即从 aio_handle 的监控中解除)，当指定
 * 时间到达后再启动异步写操作(在 timer_callback 回调中再重新将异步
 * 流的异步写操作绑定)，同时该定时器自动销毁(调用 destroy 方法)，
 * 所以如果用户继承了 aio_timer_writer 类，且子类不是在堆上分配的，
 * 则必须重载 destroy方法，同时在子类的 destroy 中执行与资源释放的
 * 相关操作，如果子类未重载 destroy，则当定时器结束后内部会自动调用
 * 基类 aio_timer_writer 的 destroy--该类调用了 delete this，此时就
 * 会导致非法内存释放操作)
 * 
 */
class ACL_CPP_API aio_timer_writer : public aio_timer_callback
{
public:
	aio_timer_writer(void);

	/**
	 * 在 aio_istream 中调用此函数以释放类对象，子类应该实现该函数
	 */
	virtual void destroy(void)
	{
		delete this;
	}

protected:
	virtual ~aio_timer_writer(void);

	/**
	 * 延迟读数据时的回调函数，从 aio_timer_callback 类中继承而来
	 */
	virtual void timer_callback(unsigned int id);
private:
	friend class aio_ostream;

	aio_ostream* out_;
	//int   write_delayed_;
	acl::string buf_;
};

/**
 * 异步写数据流类定义，该类只能在堆上被实例化，在析构时需要调用 close
 * 函数以释放该类对象
 */
class ACL_CPP_API aio_ostream : virtual public aio_stream
{
public:
	/**
	 * 构造函数
	 * @param handle {aio_handle*} 异步事件引擎句柄
	 */
	aio_ostream(aio_handle* handle);

	/**
	 * 构造函数，创建异步写流对象，并 hook 写过程及关闭/超时过程
	 * @param handle {aio_handle*} 异步事件引擎句柄
	 * @param fd {int} 连接套接口句柄
	 */
#if defined(_WIN32) || defined(_WIN64)
	aio_ostream(aio_handle* handle, SOCKET fd);
#else
	aio_ostream(aio_handle* handle, int fd);
#endif

	/**
	 * 添加异可写时的回调类对象指针，如果该回调类对象已经存在，则只是
	 * 使该对象处于打开可用状态
	 * @param callback {aio_callback*} 继承 aio_callback 的子类回调类对象，
	 *  当异步流有数据时会先调用此回调类对象中的 write_callback 接口
	 */
	void add_write_callback(aio_callback* callback);

	/**
	 * 从写回调对象集合中删除
	 * @param callback {aio_callback*} 被删除的写回调对象，
	 *  若该值为空，则删除所有的回调写对象
	 * @return {int} 返回被从回调对象集合中删除的回调对象的个数
	 */
	int del_write_callback(aio_callback* callback = NULL);

	/**
	 * 禁止回调对象类集合中的某个回调类对象，但并不从回调类对象
	 * 集合中删除，只是不被调用而已
	 * @param callback {aio_callback*} 被禁用的写回调对象，
	 *  若该值为空，则禁用所有的写回调对象
	 * @return {int} 返回被从回调对象集合中禁用的回调对象的个数
	 */
	int disable_write_callback(aio_callback* callback = NULL);

	/**
	 * 启用所有的回调对象被调用
	 * @param callback {aio_callback*} 启用指定的写回调对象，
	 *  如果该值为空，则启用所有的写回调对象
	 * @return {int} 返回被启用的写回调对象的个数
	 */
	int enable_write_callback(aio_callback* callback = NULL);

	/**
	 * 异步写规定字节数的数据，当完全写成功或出错或超时时会
	 * 调用用户注册的回调函数，在延迟异步写时，当在一个函数
	 * 内连续调用此过程时，每个延迟异步写操作会被加入延迟写
	 * 的队列中，以保证每个延迟异步写操作都可在各自的定时器
	 * 到达时被执行
	 * @param data {const void*} 数据地址
	 * @param len {int} 数据长度
	 * @param delay {int64} 如果该值 > 0 则采用延迟发送的模式(单位为微秒)
	 * @param callback {aio_timer_writer*} 定时器到达时的回调函数类对象，
	 */
#if defined(_WIN32) || defined(_WIN64)
	void write(const void* data, int len, __int64 delay = 0,
		aio_timer_writer* callback = NULL);
#else
	void write(const void* data, int len, long long int delay = 0,
		aio_timer_writer* callback = NULL);
#endif

	/**
	 * 格式化方式异步写数据，当完全写成功或出错或超时时会
	 * 调用用户注册的回调函数
	 * @param fmt {const char*} 格式字符串
	 */
	void format(const char* fmt, ...) ACL_CPP_PRINTF(2, 3);

	/**
	 * 格式化方式异步写数据，当完全写成功或出错或超时时会
	 * 调用用户注册的回调函数
	 * @param fmt {const char*} 格式字符串
	 * @param ap {va_list} 数据值列表
	 */
	void vformat(const char* fmt, va_list ap);

	/**
	 * 异步等待连接流可写，该函数设置异步流的写监听状态，当有可写时，
	 * 回调函数被触发，由用户自己负责数据的读取
	 * @param timeout {int} 写超时时间(秒)，当该值为 0 时，则没有写超时
	 */
	void write_wait(int timeout = 0);

	/**
	 * 禁止异步流的异步写状态，则将该异步流从异步引擎的监控
	 * 事件中移除，直到用户调用任何一个写操作时会自动打开异
	 * 步写状态(此时该流会重新被异步引擎监控)
	 */
	void disable_write(void);
protected:
	virtual ~aio_ostream(void);

	/**
	 * 释放动态类对象的虚函数
	 */
	virtual void destroy(void);

	/**
	 * hook 写过程
	 */
	void hook_write(void);

private:
	friend class aio_timer_writer;
	std::list<aio_timer_writer*>* timer_writers_;
	std::list<AIO_CALLBACK*> write_callbacks_;

	static int write_callback(ACL_ASTREAM*, void*);
	static int write_wakup(ACL_ASTREAM*, void*);
};

}  // namespace acl
