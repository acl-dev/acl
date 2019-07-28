#pragma once
#include "IConnection.h"

/**
 * 服务端连接对象
 */
class StatusConnection : public IConnection
{
public:
	StatusConnection(acl::aio_socket_stream* conn);
	~StatusConnection() {}

	/**
	 * 关闭服务端连接，当连接关闭时会触发 ServiceIOCallback 中的
	 * close_callback 过程，同时在 ServiceIOCallback 对象的析构过程
	 * 中会删除服务端本服务端连接对象
	 */
	void close();

protected:
	void run();

private:
};
