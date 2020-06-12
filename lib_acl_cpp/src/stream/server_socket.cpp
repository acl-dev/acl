#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/stream/server_socket.hpp"
#endif

#define	SAFE_COPY	ACL_SAFE_STRNCPY

namespace acl {

#if 0
server_socket::server_socket(int backlog, bool block)
: backlog_(backlog)
, unix_sock_(false)
, fd_(ACL_SOCKET_INVALID)
, fd_local_(ACL_SOCKET_INVALID)
{
	open_flag_ = block ? ACL_BLOCKING : ACL_NON_BLOCKING;
}
#endif

server_socket::server_socket(unsigned flag, int backlog)
: backlog_(backlog)
, unix_sock_(false)
, fd_(ACL_SOCKET_INVALID)
, fd_local_(ACL_SOCKET_INVALID)
{
	open_flag_ = 0;
	if (flag & OPEN_FLAG_NONBLOCK) {
		open_flag_ |= ACL_NON_BLOCKING;
	} else {
		open_flag_ |= ACL_BLOCKING;
	}

	if (flag & OPEN_FLAG_REUSEPORT) {
		open_flag_ |= ACL_INET_FLAG_REUSEPORT;
	}
	if (flag & OPEN_FLAG_EXCLUSIVE) {
		open_flag_ |= ACL_INET_FLAG_EXCLUSIVE;
	}
}

server_socket::server_socket(ACL_VSTREAM* sstream)
: fd_(ACL_VSTREAM_SOCK(sstream))
, fd_local_(ACL_SOCKET_INVALID)
{
	if (fd_ != ACL_SOCKET_INVALID) {
		addr_ = ACL_VSTREAM_LOCAL(sstream);
	}
}

server_socket::server_socket(ACL_SOCKET fd)
: fd_(fd)
, fd_local_(ACL_SOCKET_INVALID)
{
	open_flag_ = 0;
	char buf[512];
	if (fd_ != ACL_SOCKET_INVALID) {
		if (acl_getsockname(fd_, buf, sizeof(buf)) == 0) {
			addr_ = buf;
		}
	}
}

server_socket::server_socket(void)
: backlog_(128)
, unix_sock_(false)
, fd_(ACL_SOCKET_INVALID)
, fd_local_(ACL_SOCKET_INVALID)
{
	open_flag_ = 0;
}

server_socket::~server_socket(void)
{
	if (fd_local_ != ACL_SOCKET_INVALID) {
		acl_socket_close(fd_local_);
	}
}

bool server_socket::opened(void) const
{
	return fd_ != ACL_SOCKET_INVALID;
}

bool server_socket::open(const char* addr)
{
	if (fd_ != ACL_SOCKET_INVALID) {
		logger_error("listen fd already opened");
		return true;
	}

#ifdef ACL_UNIX
	if (acl_valid_unix(addr)) {
		fd_ = acl_unix_listen(addr, backlog_, open_flag_);
		unix_sock_ = true;
		addr_ = addr;
	} else
#endif
	{
		fd_ = acl_inet_listen(addr, backlog_, open_flag_);
		unix_sock_ = false;
	}

	if (fd_ == ACL_SOCKET_INVALID) {
		logger_error("listen %s error %s", addr, last_serror());
		unix_sock_ = false;
		addr_ = addr;
		return false;
	}

	// 保留由本类对象产生的句柄，以便于在析构函数中将其关闭
	fd_local_ = fd_;

	if (unix_sock_) {
		return true;
	}

	// 之所以再用 getsockname 重新获得一些监听地址，主要是为了应对当输入
	// 的 addr 为 ip:0 的情形，即当给定的地址中的端口为 0 时要求操作系统
	// 自动分配一个端口号
	char buf[512];
	if (acl_getsockname(fd_, buf, sizeof(buf)) < 0) {
		logger_error("getsockname error: %s", acl_last_serror());
		addr_ = addr;
	} else {
		addr_ = buf;
	}
	return true;
}

ACL_SOCKET server_socket::unbind(void)
{
	ACL_SOCKET sock = fd_local_;
	fd_local_ = ACL_SOCKET_INVALID;
	fd_ = ACL_SOCKET_INVALID;
	return sock;
}

bool server_socket::close(void)
{
	if (fd_local_ == ACL_SOCKET_INVALID) {
		return true;
	}

	bool ret = acl_socket_close(fd_local_) == 0 ? true : false;
	fd_ = ACL_SOCKET_INVALID;
	fd_local_ = ACL_SOCKET_INVALID;
	addr_.clear();
	return ret;
}

socket_stream* server_socket::accept(int timeout /* = 0 */,
	bool* etimed /* = NULL */)
{
	if (etimed) {
		*etimed = false;
	}

	if (fd_ == ACL_SOCKET_INVALID) {
		logger_error("server socket not opened!");
		return NULL;
	}

	// if ((open_flag_ & ACL_NON_BLOCKING) && timeout > 0)
	if (timeout > 0) {
		if (acl_read_wait(fd_, timeout) == -1) {
			if (etimed) {
				*etimed = true;
			}
			return NULL;
		}
	}

	ACL_SOCKET fd = acl_accept(fd_, NULL, 0, NULL);
	if (fd == ACL_SOCKET_INVALID) {
		if (open_flag_ & ACL_NON_BLOCKING) {
			logger_error("accept error %s", last_serror());
		} else if (last_error() != ACL_EAGAIN
			&& last_error() != ACL_EWOULDBLOCK) {

			logger_error("accept error %s", last_serror());
		}
		return NULL;
	}

	socket_stream* client = NEW socket_stream();
	if (client->open(fd) == false) {
		logger_error("create socket_stream error!");
		return NULL;
	}

	if (!unix_sock_) {
		acl_tcp_set_nodelay(fd);
	}

	return client;
}

void server_socket::set_tcp_defer_accept(int timeout)
{
	if (fd_ == ACL_SOCKET_INVALID) {
		logger_error("server socket not opened!");
	} else {
		acl_tcp_defer_accept(fd_, timeout);
	}
}

} // namespace acl
