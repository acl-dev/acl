#pragma once

#include "acl_cpp/stream/aio_socket_stream.hpp"

typedef struct
{
	char  addr[64];
	acl::aio_handle* handle;
	int   connect_timeout;
	int   read_timeout;
	int   nopen_limit;
	int   nopen_total;
	int   nwrite_limit;
	int   nwrite_total;
	int   nread_total;
	int   id_begin;
	bool  debug;
} IO_CTX;

class CConnectClientCallback : public acl::aio_open_callback
{
public:
	CConnectClientCallback(IO_CTX* ctx, acl::aio_socket_stream* client, int id);
	~CConnectClientCallback();

	/**
	 * 基类虚函数, 当异步流读到所要求的数据时调用此回调函数
	 * @param data {char*} 读到的数据地址
	 * @param len {int｝ 读到的数据长度
	 * @return {bool} 返回给调用者 true 表示继续，否则表示需要关闭异步流
	 */
	bool read_callback(char* data, int len);

	/**
	 * 基类虚函数, 当异步流写成功时调用此回调函数
	 * @return {bool} 返回给调用者 true 表示继续，否则表示需要关闭异步流
	 */
	bool write_callback();

	/**
	 * 基类虚函数, 当该异步流关闭时调用此回调函数
	 */
	void close_callback();

	/**
	 * 基类虚函数，当异步流超时时调用此函数
	 * @return {bool} 返回给调用者 true 表示继续，否则表示需要关闭异步流
	 */
	bool timeout_callback();

	/**
	 * 基类虚函数, 当异步连接成功后调用此函数
	 * @return {bool} 返回给调用者 true 表示继续，否则表示需要关闭异步流
	 */
	bool open_callback();

	static bool CConnectClientCallback::connect_server(IO_CTX* ctx, int id);
private:
	acl::aio_socket_stream* client_;
	IO_CTX* ctx_;
	int   nwrite_;
	int   id_;
};