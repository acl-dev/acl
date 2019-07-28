#pragma once
#include "../acl_cpp_define.hpp"
#include "../stream/aio_listen_stream.hpp"

namespace acl {

class aio_handle;
class aio_listen_stream;

/**
 * 异步消息服务端，纯虚类
 */
class ACL_CPP_API ipc_server : private aio_accept_callback
{
public:
	ipc_server();

	virtual ~ipc_server();

	/**
	 * 打开异步监听服务流
	 * @param handle {aio_handle*} 异步引擎句柄，非空
	 * @param addr {const char*} 监听地址
	 * @return {bool} 监听是否成功
	 */
	bool open(aio_handle* handle, const char* addr = "127.0.0.1:0");

	/**
	 * 当 open 成功后通过此函数获得监听地址
	 * @return {const char*} 监听地址，格式为： IP:PORT
	 */
	const char* get_addr() const;

	/**
	 * 获得异步流句柄
	 * @return {aio_listen_stream*}
	 */
	aio_listen_stream* get_stream() const;

	/**
	 * 获得异步引擎句柄
	 */
	aio_handle& get_handle() const;

protected:
	/**
	 * 当监听流成功打开后的回调函数
	 * @param addr {const char*} 实际的监听地址，格式：IP:PORT
	 */
	virtual void on_open(const char*addr)
	{
		(void) addr;
	}

	/**
	 * 当监听流关闭时的回调函数
	 */
	virtual void on_close() {}

	/**
	 * 当异步监听流获得一个客户端连接后的回调函数
	 * @param client {aio_socket_stream*} 客户端 IPC 流
	 */
	virtual void on_accept(aio_socket_stream* client)
	{
		(void) client;
	}

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * 对于基于 _WIN32 窗口消息的情况，当调用 open 时，则内部
	 * 会自动调用 create_windows 过程
	 */
	virtual bool create_window()
	{
		return false;
	}
#endif

private:
	aio_handle* handle_;
	aio_listen_stream* sstream_;

	/**
	 * 基类虚函数，当有新连接到达后调用此回调过程
	 * @param client {aio_socket_stream*} 异步客户端流
	 * @return {bool} 返回 true 以通知监听流继续监听
	 */
	virtual bool accept_callback(aio_socket_stream* client);

	/**
	 * 基类虚函数，当监听流关闭时的回调过程
	 */
	virtual void close_callback();

	/**
	 * 基类虚函数，当监听流超时的回调过程
	 */
	virtual bool timeout_callback();
};

}  // namespace acl
