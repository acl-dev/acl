#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/stream/server_socket.hpp"

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
	safe_snprintf(addr_, sizeof(addr_), "%s", addr);

#ifndef WIN32
	if (strchr(addr, '/') != NULL)
	{
		fd_ = acl_unix_listen(addr, backlog_, block_
			? ACL_BLOCKING : ACL_NON_BLOCKING);
		unix_sock_ = true;
	}
	else
#endif
		fd_ = acl_inet_listen(addr, backlog_, block_
			? ACL_BLOCKING : ACL_NON_BLOCKING);

	if (fd_ == ACL_SOCKET_INVALID)
	{
		logger_error("listen %s error %s", addr, last_serror());
		unix_sock_ = false;
		return false;
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
