#pragma once
#include "IConnection.h"

/**
 * 服务端连接对象
 */
class ServerConnection : public IConnection
{
public:
	ServerConnection(acl::aio_socket_stream* conn);
	~ServerConnection() {}

	/**
	 * 设置当前服务端连接的个数
	 * @param nconns {unsigned int}
	 */
	void set_nconns(unsigned int nconns);

	/**
	 * 获得当前服务端连接的个数
	 * @return {unsigned int}
	 */
	unsigned int get_nconns() const
	{
		return nconns_;
	}

	/**
	 * 当前服务端连接个数加 1
	 */
	void inc_nconns();

	/**
	 * 关闭服务端连接，当连接关闭时会触发 ServiceIOCallback 中的
	 * close_callback 过程，同时在 ServiceIOCallback 对象的析构过程
	 * 中会删除服务端本服务端连接对象
	 */
	void close();

protected:
	void run();

private:
	unsigned int nconns_;
};
