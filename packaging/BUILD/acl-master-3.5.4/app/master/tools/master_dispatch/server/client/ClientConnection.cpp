#include "stdafx.h"
#include "client/ManagerTimer.h"
#include "client/ClientManager.h"
#include "client/ClientConnection.h"

ClientConnection::ClientConnection(acl::aio_socket_stream* conn, int ttl)
: IConnection(conn)
{
	struct timeval now;

	gettimeofday(&now, NULL);
	expire_ = ((acl_uint64) now.tv_sec + ttl) * 1000000
		+ ((acl_uint64) now.tv_usec);
}

ClientConnection::~ClientConnection()
{
	conn_->close();
}

void ClientConnection::run()
{
	// 必须先将套接置为阻塞状态，否则接收者调用读时会立刻返回 -1
	acl_non_blocking(conn_->sock_handle(), ACL_BLOCKING);

	// 调用描述字发送过程将客户端套接字传给服务端
	if (ManagerTimer::transfer(this) == false)
		// 如果传输描述字失败，则加入待处理队列，由定时器
		// 进行处理
		ClientManager::get_instance().set(this);
	else
	{
		// 尝试从集合中删除
		ClientManager::get_instance().del(this);
		delete this;
	}
}

bool ClientConnection::expired() const
{
	struct timeval now;

	gettimeofday(&now, NULL);
	long long present = ((acl_uint64) now.tv_sec) * 1000000
		+ ((acl_uint64) now.tv_usec);

	return present >= expire_ ? true : false;
}
