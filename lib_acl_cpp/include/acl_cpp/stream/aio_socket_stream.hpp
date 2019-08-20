#pragma once
#include "../acl_cpp_define.hpp"
#if defined(_WIN32) || defined(_WIN64)
# include <WinSock2.h>
#endif
#include "aio_istream.hpp"
#include "aio_ostream.hpp"

namespace acl
{

/**
 * 当异步客户端流异步连接远程服务器时的回调函数类，该类为纯虚类，
 * 要求子类必须实现 open_callback 回调过程
 */
class ACL_CPP_API aio_open_callback : public aio_callback
{
public:
	aio_open_callback(void) {}
	virtual ~aio_open_callback(void) {}

	virtual bool open_callback(void) = 0;
protected:
private:
};

struct AIO_OPEN_CALLBACK 
{
	aio_open_callback* callback;
	bool enable;
};

class aio_handle;

/**
 * 网络异步流类，该类继承了异步读写流，同时该类只能在堆上分配，
 * 不能在栈上分配，并且该类结束时应用不必释放该类对象，因为异步流
 * 框架内部会自动释放该类对象，应用可以调用 close 主动关闭流
 */
class ACL_CPP_API aio_socket_stream
	: public aio_istream
	, public aio_ostream
{
public:
	/**
	 * 构造函数，创建网络异步客户端流
	 * @param handle {aio_handle*} 异步引擎句柄
	 * @param stream {ACL_ASTREAM*} 非阻塞流
	 * @param opened {bool} 该流是否已经与服务端正常建立了连接，如果是则自动
	 *  hook 读写过程及关闭/超时过程，否则仅 hook 关闭/超时过程
	 */
	aio_socket_stream(aio_handle* handle, ACL_ASTREAM* stream, bool opened = false);

	/**
	 * 构造函数，创建网络异步客户端流，并 hook 读写过程及关闭/超时过程
	 * @param handle {aio_handle*} 异步引擎句柄
	 * @param fd {int} 连接套接口句柄
	 */
#if defined(_WIN32) || defined(_WIN64)
	aio_socket_stream(aio_handle* handle, SOCKET fd);
#else
	aio_socket_stream(aio_handle* handle, int fd);
#endif

	/**
	 * 打开与远程服务器的连接，并自动 hook 流的关闭、超时以及连接成功
	 * 时的回调处理过程
	 * @param handle {aio_handle*} 异步引擎句柄
	 * @param addr {const char*} 远程服务器的地址，地址格式为：
	 *  针对TCP：IP:Port 或 针对域套接口：{filePath}
	 * @param timeout {int} 连接超时时间(秒)
	 * @return {bool} 如果连接立即返回失败则该函数返回 false，如果返回
	 *  true 只是表示正处于连接过程中，至于连接是否超时或连接是否失败
	 *  应通过回调函数来判断
	 */
	static aio_socket_stream* open(aio_handle* handle,
		const char* addr, int timeout);

	/**
	 * 添加针对 open 函数的回调过程
	 * @param callback {aio_open_callback*} 回调函数
	 */
	void add_open_callback(aio_open_callback* callback);

	/**
	 * 从 open 回调对象集合中删除
	 * @param callback {aio_open_callback*} 被删除的回调对象，若该
	 *  值为空，则删除所有的回调对象
	 * @return {int} 返回被从回调对象集合中删除的回调对象的个数
	 */
	int del_open_callback(aio_open_callback* callback = NULL);

	/**
	 * 禁止回调对象类集合中的某个回调类对象，但并不从回调类对象
	 * 集合中删除，只是不被调用而已
	 * @param callback {aio_open_callback*} 被禁止的回调对象，若该
	 *  值为空，则禁止所有的回调对象
	 * @return {int} 返回被从回调对象集合中禁用的回调对象的个数
	 */
	int disable_open_callback(aio_open_callback* callback = NULL);

	/**
	 * 启用所有的回调对象被调用
	 * @param callback {aio_open_callback*} 启用指定的回调对象，
	 * 如果该值为空，则启用所有的回调对象
	 * @return {int} 返回被启用的回调对象的个数
	 */
	int enable_open_callback(aio_open_callback* callback = NULL);

	/**
	 * 针对 open 过程，判断是否已经连接成功
	 * @return {bool} 返回 true 表示连接成功，否则表示还连接成功
	 */
	bool is_opened(void) const;

protected:
	virtual ~aio_socket_stream(void);

	/**
	 * 通过此函数来动态释放只能在堆上分配的异步流类对象
	 */
	virtual void destroy(void);

	/**
	 * 注册流连接成功的回调过程
	 */
	void hook_open(void);

private:
	std::list<AIO_OPEN_CALLBACK*>* open_callbacks_;

	static int open_callback(ACL_ASTREAM*, void*);
};

}  // namespace acl
