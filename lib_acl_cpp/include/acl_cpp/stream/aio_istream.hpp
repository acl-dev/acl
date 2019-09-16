#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include "aio_handle.hpp"
#include "aio_timer_callback.hpp"
#include "aio_stream.hpp"

namespace acl
{

class aio_istream;

/**
 * 延迟异步读数据类，基类为 aio_timer_callback (see aio_handle.hpp)，
 * 所谓延迟异步读，就是把异步读流(aio_istream)放在定时器中，将该异
 * 步流的异步读操作解除绑定(即从 aio_handle 的监控中解除)，当指定
 * 时间到达后再启动异步读操作(在 timer_callback 回调中再重新将异步
 * 流的异步读操作绑定)，同时该定时器自动销毁(调用 destroy 方法)，
 * 所以如果用户继承了 aio_timer_reader 类，且子类不是在堆上分配的，
 * 则必须重载 destroy方法，同时在子类的 destroy 中执行与资源释放的
 * 相关操作，如果子类未重载 destroy，则当定时器结束后内部会自动调用
 * 基类 aio_timer_reader 的 destroy--该类调用了 delete this，此时就
 * 会导致非法内存释放操作)
 * 
 */
class ACL_CPP_API aio_timer_reader : public aio_timer_callback
{
public:
	aio_timer_reader(void) {}

	/**
	 * 在 aio_istream 中调用此函数以释放类对象，子类应该实现该函数
	 */
	virtual void destroy(void)
	{
		delete this;
	}
protected:
	virtual ~aio_timer_reader(void) {}
	/**
	 * 延迟读数据时的回调函数，从 aio_timer_callback 类中继承而来
	 */
	virtual void timer_callback(unsigned int id);
private:
	// 允许 aio_istream 可以直接修改本类的私有成员变量
	friend class aio_istream;

	aio_istream* in_;
	//int   read_delayed_;
	bool  delay_gets_;
	int   delay_timeout_;
	bool  delay_nonl_;
	int   delay_count_;
};

/**
 * 异步读数据流类定义，该类只能在堆上被实例化，在析构时需要调用 close
 * 函数以释放该类对象
 */
class ACL_CPP_API aio_istream : virtual public aio_stream
{
public:
	/**
	 * 构造函数
	 * @param handle {aio_handle*} 异步事件引擎句柄
	 */
	aio_istream(aio_handle* handle);

	/**
	 * 构造函数，创建异步读流对象，并 hook 读过程及关闭/超时过程
	 * @param handle {aio_handle*} 异步事件引擎句柄
	 * @param fd {int} 连接套接口句柄
	 */
#if defined(_WIN32) || defined(_WIN64)
	aio_istream(aio_handle* handle, SOCKET fd);
#else
	aio_istream(aio_handle* handle, int fd);
#endif

	/**
	 * 添加异可读时的回调类对象指针，如果该回调类对象已经存在，则只是
	 * 使该对象处于打开可用状态
	 * @param callback {aio_callback*} 继承 aio_callback 的子类回调类对象，
	 *  当异步流有数据时会先调用此回调类对象中的 read_callback 接口
	 */
	void add_read_callback(aio_callback* callback);

	/**
	 * 从读回调对象集合中删除
	 * @param callback {aio_read_callback*} 被删除的回调对象，
	 * 若该值为空，则删除所有的回调对象
	 * @return {int} 返回被从回调对象集合中删除的回调对象的个数
	 */

	/**
	 * 从读回调对象集合中删除回调对象
	 * @param callback {aio_callback*} 从 aio_callback 继承的子类对象指针，
	 *  若该值为空，则删除所有的读回调对象
	 * @return {int} 返回被从回调对象集合中删除的回调对象的个数
	 */
	int del_read_callback(aio_callback* callback = NULL);

	/**
	 * 禁止回调对象类集合中的某个回调类对象，但并不从回调类对象
	 * 集合中删除，只是不被调用而已
	 * @param callback {aio_callback*} 从 aio_callback 继承的子类对象指针，
	 *  若该值为空，则禁止所有的读回调对象
	 * @return {int} 返回被从回调对象集合中禁用的回调对象的个数
	 */
	int disable_read_callback(aio_callback* callback = NULL);

	/**
	 * 启用所有的回调对象被调用
	 * @param callback {aio_callback*} 从 aio_callback 继承的子类对象指针，
	 *  若该值为空，则启用所有的读回调对象
	 * @return {int} 返回被启用的回调对象的个数
	 */
	int enable_read_callback(aio_callback* callback = NULL);

	/**
	 * 异步读取一行数据，当延迟异步读时，如果连续调用此过程，
	 * 则只有最后一个延迟读操作生效
	 * @param timeout {int} 读超时时间(秒)，若为 0 则表示
	 *  永远等待直到读到完整一行数据或出错
	 * @param nonl {bool} 是否自动去掉尾部的回车换行符
	 * @param delay {int64} 如果对方发送数据比较快时，此参数
	 *  大于 0 时可以延迟接收对方的数据，该值控制延迟读数据
	 *  的时间(单位为微秒)
	 * @param callback {aio_timer_reader*} 定时器到达时的回调函数类对象，
	 *  当 delay > 0，如果该值为空，则采用缺省的对象
	 */
#if defined(_WIN32) || defined(_WIN64)
	void gets(int timeout = 0, bool nonl = true,
		__int64 delay = 0, aio_timer_reader* callback = NULL);
#else
	void gets(int timeout = 0, bool nonl = true,
		long long int delay = 0, aio_timer_reader* callback = NULL);
#endif

	/**
	 * 异步读取数据，当延迟异步读时，如果连续调用此过程，
	 * 则只有最后一个延迟读操作生效
	 * @param count {int} 所要求读到的数据量，如果为 0 则只要有数据
	 *  可读就返回，否则直到读超时或读出错或读满足所要求的字节数
	 * @param timeout {int} 读超时时间(秒)，若为 0 则表示
	 *  永远等待直到读到所要求的数据或出错
	 * @param delay {int64} 如果对方发送数据比较快时，此参数
	 *  大于 0 时可以延迟接收对方的数据，该值控制延迟读数据
	 *  的时间(单位为微秒)
	 * @param callback {aio_timer_reader*} 定时器到达时的回调函数类对象，
	 *  如果该值为空，则采用缺省的对象
	 */
#if defined(_WIN32) || defined(_WIN64)
	void read(int count = 0, int timeout = 0,
		__int64 delay = 0, aio_timer_reader* callback = NULL);
#else
	void read(int count = 0, int timeout = 0,
		long long int delay = 0, aio_timer_reader* callback = NULL);
#endif

	/**
	 * 异步等待连接流可读，该函数设置异步流的读监听状态，当有数据可读
	 * 时，回调函数被触发，由用户自己负责数据的读取
	 * @param timeout {int} 读超时时间(秒)，当该值为 0 时，则没有读超时
	 */
	void read_wait(int timeout = 0);

	/**
	 * 禁止异步流的异步读状态，将该异步流从异步引擎的监控中
	 * 移除，直到用户调用任何一个异步读操作(此时，异步引擎会
	 * 自动重新监控该流的可读状态)
	 */
	void disable_read(void);

	/**
	 * 设置流是否采用连接读功能
	 * @param onoff {bool}
	 */
	void keep_read(bool onoff);

	/**
	 * 获得流是否是设置了连续读功能
	 * @return {bool}
	 */
	bool keep_read(void) const;

	/**
	 * 设置接收缓冲区的最大长度，以避免缓冲区溢出，默认值为 0 表示不限制
	 * @param max {int}
	 * @return {aio_istream&}
	 */
	aio_istream& set_buf_max(int max);

	/**
	 * 获得当前接收缓冲区的最大长度限制
	 * @return {int} 返回值  <= 0 表示没有限制
	 */
	int get_buf_max(void) const;

protected:
	virtual ~aio_istream(void);

	/**
	 * 释放动态类对象的虚函数
	 */
	virtual void destroy(void);

	/**
	 * 注册可读的回调函数
	 */
	void hook_read(void);

private:
	friend class aio_timer_reader;
	aio_timer_reader* timer_reader_;
	std::list<AIO_CALLBACK*> read_callbacks_;

	static int read_callback(ACL_ASTREAM*,  void*, char*, int);
	static int read_wakeup(ACL_ASTREAM* stream, void* ctx);
};

}  // namespace acl
