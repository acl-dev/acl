#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/stream/server_socket.hpp"
#endif

namespace acl {

server_socket::server_socket(int backlog /* = 128 */, bool block /* = true */)
: backlog_(backlog)
, block_(block)
, unix_sock_(false)
, fd_(ACL_SOCKET_INVALID)
{
}

server_socket::~server_socket()
{
	if (fd_ != ACL_SOCKET_INVALID)
		acl_socket_close(fd_);
}

bool server_socket::open(const char* addr)
{
#ifndef ACL_WINDOWS
	if (strchr(addr, '/') != NULL)
	{
		fd_ = acl_unix_listen(addr, backlog_, block_
			? ACL_BLOCKING : ACL_NON_BLOCKING);
		unix_sock_ = true;
		ACL_SAFE_STRNCPY(addr_, addr, sizeof(addr_));
	}
	else
#endif
		fd_ = acl_inet_listen(addr, backlog_, block_
			? ACL_BLOCKING : ACL_NON_BLOCKING);

	if (fd_ == ACL_SOCKET_INVALID)
	{
		logger_error("listen %s error %s", addr, last_serror());
		unix_sock_ = false;
		ACL_SAFE_STRNCPY(addr_, addr, sizeof(addr_));
		return false;
	}

	if (unix_sock_)
		return true;

	// 之所以再用 getsockname 重新获得一些监听地址，主要是为了应对当输入的 addr 
	// 为 ip:0 的情形，即当给定的地址中的端口为 0 时要求操作系统自动分配一个端口号
	if (acl_getsockname(fd_, addr_, sizeof(addr_)) < 0)
	{
		logger_error("getsockname error: %s", acl_last_serror());
		ACL_SAFE_STRNCPY(addr_, addr, sizeof(addr_));
	}
	return true;
}

bool server_socket::close()
{
	if (fd_ == ACL_SOCKET_INVALID)
		return true;
	bool ret = acl_socket_close(fd_) == 0 ? true : false;
	fd_ = ACL_SOCKET_INVALID;
	addr_[0] = 0;
	return ret;
}

socket_stream* server_socket::accept(int timeout /* = 0 */)
{
	if (fd_ == ACL_SOCKET_INVALID)
	{
		logger_error("server socket not opened!");
		return NULL;
	}

	if (block_ && timeout > 0)
	{
		if (acl_read_wait(fd_, timeout) == -1)
			return NULL;
	}

	ACL_SOCKET fd = acl_accept(fd_, NULL, 0, NULL);
	if (fd == ACL_SOCKET_INVALID)
	{
		if (block_)
			logger_error("accept error %s", last_serror());
		else if (last_error() != ACL_EAGAIN
			&& last_error() != ACL_EWOULDBLOCK)
		{
			logger_error("accept error %s", last_serror());
		}
		return NULL;
	}

	socket_stream* client = new socket_stream();
	if (client->open(fd) == false)
	{
		logger_error("create socket_stream error!");
		return NULL;
	}

	if (!unix_sock_)
		acl_tcp_set_nodelay(fd);

	return client;
}

void server_socket::set_tcp_defer_accept(int timeout)
{
	if (fd_ == ACL_SOCKET_INVALID)
		logger_error("server socket not opened!");
	else
		acl_tcp_defer_accept(fd_, timeout);
}

} // namespace acl
