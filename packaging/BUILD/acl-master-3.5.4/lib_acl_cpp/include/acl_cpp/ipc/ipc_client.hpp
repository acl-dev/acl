#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include "../stream/aio_socket_stream.hpp"

namespace acl {

typedef struct MSG_HDR 
{
	int nMsg;
	int dlen;
#if defined(_WIN32) || defined(_WIN64)
	__int64   magic;
#else
	long long int magic;
#endif
} MSG_HDR;

typedef enum
{
	IO_WAIT_HDR,
	IO_WAIT_DAT
} io_status;

class aio_handle;
class ipc_adapter;
class aio_socket_stream;
class socket_stream;

/**
 * 异步IP消息类
 */
class ACL_CPP_API ipc_client : private aio_open_callback
{
public:
#if defined(_WIN32) || defined(_WIN64)
	ipc_client(__int64 magic = -1);
#else
	ipc_client(long long int magic = -1);
#endif
	virtual ~ipc_client();

	/**
	 * 直接销毁接口，子类可以重载该接口
	 */
	virtual void destroy()
	{
		delete this;
	}

	/**
	 * 当调用 open 函数连接消息服务器成功时调用此函数
	 */
	virtual void on_open() {}

	/**
	 * 当异步流关闭时的回调接口
	 */
	virtual void on_close() {}

	/**
	 * 当收到消息时的回调函数，子类必须实现该接口
	 * @param nMsg {int} 用户添加的自定义消息值
	 * @param data {void*} 消息数据
	 * @param dlen {int} 消息数据的长度
	 */
	virtual void on_message(int nMsg, void* data, int dlen);

	/**
	 * 与消息服务器之间建立连接并创建异步流
	 * @param handle {aio_handle*} 异步引擎句柄
	 * @param addr {const char*} 消息服务器监听地址，格式为:
	 *  IP:PORT(支持_WIN32/UNIX)，unix_path (仅支持UNIX)
	 * @param timeout {int} 连接超时时间
	 */
	bool open(aio_handle* handle, const char* addr, int timeout);

	/**
	 * 异步流已经建立，调用此函数完成 ipc_client 连接过程
	 * @param client {aio_socket_stream*} 异步连接流
	 */
	void open(aio_socket_stream* client);

	/**
	 * 与消息服务器之间建立连接并创建同步流
	 * @param addr {const char*} 消息服务器监听地址，格式为:
	 *  IP:PORT(支持_WIN32/UNIX)，unix_path (仅支持UNIX)
	 * @param timeout {int} 连接超时时间
	 */
	bool open(const char* addr, int timeout);

	/**
	 * 同步流已经建立，调用此函数完成 ipc_client 连接过程
	 * @param client {socket_stream*} 异步连接流
	 */
	void open(socket_stream* client);

	/**
	 * 消息流已经创建，调用此函数打开 IPC 通道
	 */
	void wait();

	/**
	 * 主动关闭消息流
	 */
	void close();

	/**
	 * 连接流是否正常打开着
	 * @return {bool}
	 */
	bool active() const;

	/**
	 * 添加指定消息的回调过程对象
	 * @param nMsg {int} 消息号
	 */
	void append_message(int nMsg);

	/**
	 * 删除指定消息的回调过程对象
	 * @param nMsg {int} 消息号
	 */
	void delete_message(int nMsg);

	/**
	 * 发送消息
	 * @param nMsg {int} 消息号
	 * @param data {const void*} 数据
	 * @param dlen {int} 数据长度
	 */
	void send_message(int nMsg, const void* data, int dlen);

	/**
	 * 获得异步流句柄
	 * @return {aio_socket_stream*}
	 */
	aio_socket_stream* get_async_stream() const;

	/**
	 * 获得异步引擎句柄
	 */
	aio_handle& get_handle() const;

	/**
	 * 获得同步流够本
	 * @return {socket_stream*}
	 */
	socket_stream* get_sync_stream() const;
protected:
	/**
	 * 触发消息过程
	 * @param nMsg {int} 消息ID
	 * @param data {void*} 接收到的消息数据地址
	 * @param dlen {int} 接收到的消息数据长度
	 */
	void trigger(int nMsg, void* data, int dlen);
private:
#if defined(_WIN32) || defined(_WIN64)
	__int64   magic_;
#else
	long long int magic_;
#endif
	char* addr_;
	std::list<int> messages_;
	//aio_handle* handle_;
	aio_socket_stream* async_stream_;
	socket_stream* sync_stream_;
	socket_stream* sync_stream_inner_;
	bool closing_;

	io_status status_;
	MSG_HDR hdr_;

	// 基类虚函数

	virtual bool read_callback(char* data, int len);
	virtual bool write_callback();
	virtual void close_callback();
	virtual bool timeout_callback();
	virtual bool open_callback();
};

}  // namespace acl
