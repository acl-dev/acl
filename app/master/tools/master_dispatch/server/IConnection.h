#pragma once

// 纯虚类，用来处理来自于客户端及服务端的连接
class IConnection
{
public:
	IConnection(acl::aio_socket_stream* conn) : conn_(conn) {}
	virtual ~IConnection() {}

	/**
	 * 纯虚函数，子类必须实现
	 */
	virtual void run() = 0;

	/**
	 * 获得连接对象的 socket 描述符
	 * @return {int}
	 */
	int   sock_handle() const;

	/**
	 * 获得非阻塞连接流对象
	 * @return {acl::aio_socket_stream*}
	 */
	acl::aio_socket_stream* get_conn() const
	{
		return conn_;
	}

	/**
	 * 获得连接对象的地址
	 * @return {const char*}
	 */
	const char* get_peer(bool full = true) const;

protected:
	acl::aio_socket_stream* conn_;
};
