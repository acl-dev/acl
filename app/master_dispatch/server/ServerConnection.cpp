#include "stdafx.h"
#include "ServerIOCallback.h"
#include "ServerConnection.h"

ServerConnection::ServerConnection(acl::aio_socket_stream* conn)
: IConnection(conn)
, nconns_(0)
{
}

void ServerConnection::run()
{
	// 创建服务端连接 IO 处理的回调处理对象
	ServerIOCallback* callback = new ServerIOCallback(this);

	conn_->add_read_callback(callback);
	conn_->add_close_callback(callback);
	conn_->add_timeout_callback(callback);

	// 异步从服务端获取一行数据
	conn_->gets();
}

void ServerConnection::set_nconns(unsigned int nconns)
{
	nconns_ = nconns;
}

void ServerConnection::close()
{
	conn_->close();
}

void ServerConnection::inc_nconns()
{
	nconns_++;
}
