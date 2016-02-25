#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#endif

namespace acl {

socket_stream::socket_stream()
{
	dummy_[0] = 0;
	peer_ip_[0] = 0;
	local_ip_[0] = 0;
}

socket_stream::~socket_stream()
{
	close();
}

bool socket_stream::open(ACL_SOCKET fd)
{
	ACL_VSTREAM* conn = acl_vstream_fdopen(fd, O_RDWR,
		8192, 0, ACL_VSTREAM_TYPE_SOCK);
	acl_assert(conn);
	return open(conn);
}

bool socket_stream::open(const char* addr, int conn_timeout, int rw_timeout)
{
	ACL_VSTREAM* conn = acl_vstream_connect(addr, ACL_BLOCKING,
		conn_timeout, rw_timeout, 8192);
	if (conn == NULL)
		return false;

	return open(conn);
}

bool socket_stream::open(ACL_VSTREAM* vstream)
{
	// 先关闭旧的流对象
	if (stream_)
		acl_vstream_close(stream_);
	stream_ = vstream;
	eof_ = false;
	opened_ = true;
	//acl_tcp_set_nodelay(ACL_VSTREAM_SOCK(vstream));
	return true;
}

bool socket_stream::bind_udp(const char* addr, int rw_timeout /* = 0 */)
{
	if (stream_)
		acl_vstream_close(stream_);
	stream_ = acl_vstream_bind(addr, rw_timeout);
	if (stream_ == NULL)
		return false;
	eof_ = false;
	opened_ = true;
	return true;
}

ACL_SOCKET socket_stream::sock_handle() const
{
	if (stream_ == NULL)
		return ACL_SOCKET_INVALID;
	return ACL_VSTREAM_SOCK(stream_);
}

ACL_SOCKET socket_stream::unbind_sock()
{
	if (stream_ == NULL)
		return ACL_SOCKET_INVALID;
	ACL_SOCKET sock = ACL_VSTREAM_SOCK(stream_);
	stream_->fd.sock = ACL_SOCKET_INVALID;
	eof_ = true;
	opened_ = false;
	return sock;
}

const char* socket_stream::get_peer(bool full /* = false */) const
{
	if (stream_ == NULL)
		return dummy_;

	// xxx: acl_vstream 中没有对此地址赋值
	char* ptr = ACL_VSTREAM_PEER(stream_);
	if (ptr == NULL || *ptr == 0)
	{
		char  buf[64];
		if (acl_getpeername(ACL_VSTREAM_SOCK(stream_),
			buf, sizeof(buf)) == -1)
		{
			return dummy_;
		}
		acl_vstream_set_peer(stream_, buf);
	}

	if (full)
		return ACL_VSTREAM_PEER(stream_);
	else
		return get_peer_ip();
}

const char* socket_stream::get_peer_ip() const
{
	if (stream_ == NULL)
		return dummy_;

	if (peer_ip_[0] != 0)
		return peer_ip_;

	char* ptr = ACL_VSTREAM_PEER(stream_);
	if (ptr == NULL || *ptr == 0)
	{
		char  buf[64];
		if (acl_getpeername(ACL_VSTREAM_SOCK(stream_),
			buf, sizeof(buf)) == -1)
		{
			return dummy_;
		}
		acl_vstream_set_peer(stream_, buf);
	}

	return const_cast<socket_stream*> (this)->get_ip(
		ACL_VSTREAM_PEER(stream_),
		const_cast<socket_stream*> (this)->peer_ip_,
		sizeof(peer_ip_));
}

bool socket_stream::set_peer(const char* addr)
{
	if (stream_ == NULL)
	{
		logger_error("stream not opened yet!");
		return false;
	}

	acl_vstream_set_peer(stream_, addr);
	return true;
}

const char* socket_stream::get_local(bool full /* = false */) const
{
	if (stream_ == NULL)
		return dummy_;

	// xxx: acl_vstream 中没有对此地址赋值
	char* ptr = ACL_VSTREAM_LOCAL(stream_);
	if (ptr == NULL || *ptr == 0)
	{
		char  buf[256];
		if (acl_getsockname(ACL_VSTREAM_SOCK(stream_),
			buf, sizeof(buf)) == -1)
		{
			return dummy_;
		}
		acl_vstream_set_local(stream_, buf);
	}

	if (full)
		return ACL_VSTREAM_LOCAL(stream_);
	else
		return get_local_ip();
}

const char* socket_stream::get_local_ip() const
{
	if (stream_ == NULL)
		return dummy_;

	// xxx: acl_vstream 中没有对此地址赋值
	if (local_ip_[0] != 0)
		return local_ip_;

	char* ptr = ACL_VSTREAM_LOCAL(stream_);
	if (ptr == NULL || *ptr == 0)
	{
		char  buf[256];
		if (acl_getsockname(ACL_VSTREAM_SOCK(stream_),
			buf, sizeof(buf)) == -1)
		{
			return dummy_;
		}
		acl_vstream_set_local(stream_, buf);
	}

	return const_cast<socket_stream*>(this)->get_ip(
		ACL_VSTREAM_LOCAL(stream_),
		const_cast<socket_stream*>(this)->local_ip_,
		sizeof(local_ip_));
}

bool socket_stream::set_local(const char* addr)
{
	if (stream_ == NULL)
	{
		logger_error("stream not opened yet!");
		return false;
	}

	acl_vstream_set_local(stream_, addr);
	return true;
}

const char* socket_stream::get_ip(const char* addr, char* buf, size_t size)
{
	safe_snprintf(buf, size, "%s", addr);
	char* ptr = strchr(buf, ':');
	if (ptr)
		*ptr = 0;
	return buf;
}

bool socket_stream::alive(void) const
{
	if (stream_ == NULL)
		return false;
	if (acl_vstream_probe_status(stream_) == -1)
		return false;
	else
		return true;
}

socket_stream& socket_stream::set_tcp_nodelay(bool on)
{
	ACL_SOCKET sock = sock_handle();
	if (sock == ACL_SOCKET_INVALID)
	{
		logger_error("invalid socket handle");
		return *this;
	}
	acl_tcp_nodelay(sock, on ? 1 : 0);

	return *this;
}

socket_stream& socket_stream::set_tcp_solinger(bool on, int linger)
{
	ACL_SOCKET sock = sock_handle();
	if (sock == ACL_SOCKET_INVALID)
	{
		logger_error("invalid socket handle");
		return *this;
	}
	acl_tcp_so_linger(sock, on ? 1 : 0, linger);

	return *this;
}

socket_stream& socket_stream::set_tcp_sendbuf(int size)
{
	ACL_SOCKET sock = sock_handle();
	if (sock == ACL_SOCKET_INVALID)
	{
		logger_error("invalid socket handle");
		return *this;
	}
	acl_tcp_set_sndbuf(sock, size);

	return *this;
}

socket_stream& socket_stream::set_tcp_recvbuf(int size)
{
	ACL_SOCKET sock = sock_handle();
	if (sock == ACL_SOCKET_INVALID)
	{
		logger_error("invalid socket handle");
		return *this;
	}
	acl_tcp_set_rcvbuf(sock, size);

	return *this;
}

socket_stream& socket_stream::set_tcp_non_blocking(bool on)
{
	ACL_SOCKET sock = sock_handle();
	if (sock == ACL_SOCKET_INVALID)
	{
		logger_error("invalid socket handle");
		return *this;
	}
	(void) acl_non_blocking(sock, on ? ACL_NON_BLOCKING : ACL_BLOCKING);

	return *this;
}

bool socket_stream::get_tcp_nodelay()
{
	ACL_SOCKET sock = sock_handle();
	if (sock == ACL_SOCKET_INVALID)
	{
		logger_error("invalid socket handle");
		return false;
	}

	return acl_get_tcp_nodelay(sock) == 0 ? false : true;
}

int socket_stream::get_tcp_solinger()
{
	ACL_SOCKET sock = sock_handle();
	if (sock == ACL_SOCKET_INVALID)
	{
		logger_error("invalid socket handle");
		return -1;
	}

	return acl_get_tcp_solinger(sock);
}

int socket_stream::get_tcp_sendbuf()
{
	ACL_SOCKET sock = sock_handle();
	if (sock == ACL_SOCKET_INVALID)
	{
		logger_error("invalid socket handle");
		return -1;
	}

	return acl_tcp_get_sndbuf(sock);
}

int socket_stream::get_tcp_recvbuf()
{
	ACL_SOCKET sock = sock_handle();
	if (sock == ACL_SOCKET_INVALID)
	{
		logger_error("invalid socket handle");
		return -1;
	}

	return acl_tcp_get_rcvbuf(sock);
}

bool socket_stream::get_tcp_non_blocking()
{
	ACL_SOCKET sock = sock_handle();
	if (sock == ACL_SOCKET_INVALID)
	{
		logger_error("invalid socket handle");
		return false;
	}

	return acl_is_blocking(sock) == 0 ? true : false;
}

} // namespace acl
