#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/socket_stream.hpp"

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
	if (m_pStream)
		acl_vstream_close(m_pStream);
	m_pStream = vstream;
	m_bEof = false;
	m_bOpened = true;
	//acl_tcp_set_nodelay(ACL_VSTREAM_SOCK(vstream));
	return true;
}

bool socket_stream::bind_udp(const char* addr, int rw_timeout /* = 0 */)
{
	if (m_pStream)
		acl_vstream_close(m_pStream);
	m_pStream = acl_vstream_bind(addr, rw_timeout);
	m_bEof = false;
	m_bOpened = true;
	return true;
}

bool socket_stream::close()
{
	if (m_bOpened == false)
		return false;
	if (m_pStream == NULL)
		return true;

	m_bEof = true;
	m_bOpened = false;

	int   ret = acl_vstream_close(m_pStream);
	m_pStream = NULL;

	return (ret == 0 ? true : false);
}

ACL_SOCKET socket_stream::sock_handle() const
{
	if (m_pStream == NULL)
		return ACL_SOCKET_INVALID;
	return ACL_VSTREAM_SOCK(m_pStream);
}

ACL_SOCKET socket_stream::unbind_sock()
{
	if (m_pStream == NULL)
		return ACL_SOCKET_INVALID;
	ACL_SOCKET sock = ACL_VSTREAM_SOCK(m_pStream);
	m_pStream->fd.sock = ACL_SOCKET_INVALID;
	m_bEof = true;
	m_bOpened = false;
	return sock;
}

const char* socket_stream::get_peer(bool full /* = false */) const
{
	if (m_pStream == NULL)
	{
		const_cast<socket_stream*> (this)->dummy_[0] = 0;
		return dummy_;
	}

	// xxx: acl_vstream 中没有对此地址赋值
	char* ptr = ACL_VSTREAM_PEER(m_pStream);
	if (ptr == NULL || *ptr == 0)
	{
		char  buf[64];
		if (acl_getpeername(ACL_VSTREAM_SOCK(m_pStream),
			buf, sizeof(buf)) == -1)
		{
			return dummy_;
		}
		acl_vstream_set_peer(m_pStream, buf);
	}

	if (full)
		return ACL_VSTREAM_PEER(m_pStream);
	else
		return get_peer_ip();
}

const char* socket_stream::get_peer_ip() const
{
	if (m_pStream == NULL)
	{
		const_cast<socket_stream*> (this)->dummy_[0] = 0;
		return dummy_;
	}

	if (peer_ip_[0] != 0)
		return peer_ip_;

	char* ptr = ACL_VSTREAM_PEER(m_pStream);
	if (ptr == NULL || *ptr == 0)
	{
		char  buf[64];
		if (acl_getpeername(ACL_VSTREAM_SOCK(m_pStream),
			buf, sizeof(buf)) == -1)
		{
			return dummy_;
		}
		acl_vstream_set_peer(m_pStream, buf);
	}

	return const_cast<socket_stream*> (this)->get_ip(
		ACL_VSTREAM_PEER(m_pStream),
		const_cast<socket_stream*> (this)->peer_ip_,
		sizeof(peer_ip_));
}

bool socket_stream::set_peer(const char* addr)
{
	if (m_pStream == NULL)
	{
		logger_error("stream not opened yet!");
		return false;
	}

	acl_vstream_set_peer(m_pStream, addr);
	return true;
}

const char* socket_stream::get_local(bool full /* = false */) const
{
	if (m_pStream == NULL)
	{
		const_cast<socket_stream*> (this)->dummy_[0] = 0;
		return dummy_;
	}

	// xxx: acl_vstream 中没有对此地址赋值
	char* ptr = ACL_VSTREAM_LOCAL(m_pStream);
	if (ptr == NULL || *ptr == 0)
	{
		char  buf[256];
		if (acl_getsockname(ACL_VSTREAM_SOCK(m_pStream),
			buf, sizeof(buf)) == -1)
		{
			return dummy_;
		}
		acl_vstream_set_local(m_pStream, buf);
	}

	if (full)
		return ACL_VSTREAM_LOCAL(m_pStream);
	else
		return get_local_ip();
}

const char* socket_stream::get_local_ip() const
{
	if (m_pStream == NULL)
	{
		const_cast<socket_stream*> (this)->dummy_[0] = 0;
		return dummy_;
	}

	// xxx: acl_vstream 中没有对此地址赋值
	if (local_ip_[0] != 0)
		return local_ip_;

	char* ptr = ACL_VSTREAM_LOCAL(m_pStream);
	if (ptr == NULL || *ptr == 0)
	{
		char  buf[256];
		if (acl_getsockname(ACL_VSTREAM_SOCK(m_pStream),
			buf, sizeof(buf)) == -1)
		{
			return dummy_;
		}
		acl_vstream_set_local(m_pStream, buf);
	}

	return const_cast<socket_stream*>(this)->get_ip(
		ACL_VSTREAM_LOCAL(m_pStream),
		const_cast<socket_stream*>(this)->local_ip_,
		sizeof(local_ip_));
}

bool socket_stream::set_local(const char* addr)
{
	if (m_pStream == NULL)
	{
		logger_error("stream not opened yet!");
		return false;
	}

	acl_vstream_set_local(m_pStream, addr);
	return true;
}

const char* socket_stream::get_ip(const char* addr, char* buf, size_t size)
{
	snprintf(buf, size, "%s", addr);
	char* ptr = strchr(buf, ':');
	if (ptr)
		*ptr = 0;
	return buf;
}

} // namespace acl
