#include "acl_stdafx.hpp"
#ifdef HAS_POLARSSL
# include "polarssl/ssl.h"
# include "polarssl/havege.h"
#endif
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/socket_stream.hpp"

namespace acl {

socket_stream::socket_stream()
{
	dummy_[0] = 0;
	peer_ip_[0] = 0;
	local_ip_[0] = 0;
	ssl_ = NULL;
	ssn_ = NULL;
	hs_ = NULL;
}

socket_stream::~socket_stream()
{
	close_ssl();
	close();
}

void socket_stream::close_ssl()
{
#ifdef HAS_POLARSSL
	if (ssl_)
	{
		ssl_free((ssl_context*) ssl_);
		acl_myfree(ssl_);
		ssl_ = NULL;
	}
	if (ssn_)
	{
		acl_myfree(ssn_);
		ssn_ = NULL;
	}
	if (hs_)
	{
		acl_myfree(hs_);
		hs_ = NULL;
	}
#endif
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
	eof_ = false;
	opened_ = true;
	return true;
}

bool socket_stream::close()
{
	if (opened_ == false)
		return false;
	if (stream_ == NULL)
		return true;

	eof_ = true;
	opened_ = false;

	int   ret = acl_vstream_close(stream_);
	stream_ = NULL;

	return ret == 0 ? true : false;
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
	snprintf(buf, size, "%s", addr);
	char* ptr = strchr(buf, ':');
	if (ptr)
		*ptr = 0;
	return buf;
}

bool socket_stream::open_ssl_client(void)
{
	if (stream_ == NULL)
	{
		logger_error("stream_ null");
		return false;
	}

#ifdef HAS_POLARSSL
	// 如果打开已经是 SSL 模式的流，则直接返回
	if (ssl_ != NULL)
	{
		acl_assert(ssn_);
		acl_assert(hs_);
		return true;
	}

	ssl_ = acl_mycalloc(1, sizeof(ssl_context));
	ssn_ = acl_mycalloc(1, sizeof(ssl_session));
	hs_  = acl_mymalloc(sizeof(havege_state));

	// Initialize the RNG and the session data
	::havege_init((havege_state*) hs_);

	int   ret;

	// Setup stuff
	if ((ret = ssl_init((ssl_context*) ssl_)) != 0)
	{
		logger_error("failed, ssl_init returned %d", ret);
		return false;
	}

	ssl_set_endpoint((ssl_context*) ssl_, SSL_IS_CLIENT);
	ssl_set_authmode((ssl_context*) ssl_, SSL_VERIFY_NONE);

	ssl_set_rng((ssl_context*) ssl_, havege_random, hs_);
	//ssl_set_dbg(ssl_, my_debug, stdout);
	ssl_set_bio((ssl_context*) ssl_, sock_read, this, sock_send, this);

	const int* cipher_suites = ssl_list_ciphersuites();
	if (cipher_suites == NULL)
	{
		logger_error("ssl_list_ciphersuites null");
		return false;
	}

	ssl_set_ciphersuites((ssl_context*) ssl_, cipher_suites);
	ssl_set_session((ssl_context*) ssl_, (ssl_session*) ssn_);

	acl_vstream_ctl(stream_,
		ACL_VSTREAM_CTL_READ_FN, ssl_read,
		ACL_VSTREAM_CTL_WRITE_FN, ssl_send,
		ACL_VSTREAM_CTL_CTX, this,
		ACL_VSTREAM_CTL_END);
	acl_tcp_set_nodelay(ACL_VSTREAM_SOCK(stream_));

	// Handshake
	while((ret = ssl_handshake((ssl_context*) ssl_)) != 0)
	{
		if (ret != POLARSSL_ERR_NET_WANT_READ
			&& ret != POLARSSL_ERR_NET_WANT_WRITE)
		{
			logger_error("ssl_handshake failed: 0x%x", ret);
			return false;
		}
	}

	return true;
#else
	logger_error("define HAS_POLARSSL first!");
	return false;
#endif
}


int socket_stream::sock_read(void *ctx, unsigned char *buf, size_t len)
{
#ifdef HAS_POLARSSL
	socket_stream* cli = (socket_stream*) ctx;
	ACL_VSTREAM* stream = cli->get_vstream();
	acl_assert(stream);
	int   ret, timeout = 120;

	if ((ret = acl_socket_read(ACL_VSTREAM_SOCK(stream), buf, len,
		timeout, stream, NULL)) < 0)
	{
		int   errnum = acl_last_error();
		if (ret == ACL_EINTR || ret == ACL_EWOULDBLOCK
#if ACL_EWOULDBLOCK != ACL_EAGAIN
			|| ret == ACL_EAGAIN
#endif
			)
			return POLARSSL_ERR_NET_WANT_READ;
		else if (errnum == ACL_ECONNRESET || errno == EPIPE)
			return POLARSSL_ERR_NET_CONN_RESET;
		else
			return POLARSSL_ERR_NET_RECV_FAILED;
	}

	return ret;
#else
	(void) ctx;
	(void) buf;
	(void) len;
	return -1;
#endif
}

int socket_stream::sock_send(void *ctx, const unsigned char *buf, size_t len)
{
#ifdef HAS_POLARSSL
	socket_stream* cli = (socket_stream*) ctx;
	ACL_VSTREAM* stream = cli->get_vstream();
	acl_assert(stream);
	int   ret, timeout = 120;

	if ((ret = acl_socket_write(ACL_VSTREAM_SOCK(stream), buf, len,
		timeout, stream, NULL)) < 0)
	{
		int   errnum = acl_last_error();
		if (ret == ACL_EINTR || ret == ACL_EWOULDBLOCK
#if ACL_EWOULDBLOCK != ACL_EAGAIN
			|| ret == ACL_EAGAIN
#endif
			)
			return POLARSSL_ERR_NET_WANT_WRITE;
		else if (errnum == ACL_ECONNRESET || errno == EPIPE)
			return POLARSSL_ERR_NET_CONN_RESET;
		else
			return POLARSSL_ERR_NET_SEND_FAILED;
	}

	return ret;
#else
	(void) ctx;
	(void) buf;
	(void) len;
	return -1;
#endif
}

int socket_stream::ssl_read(ACL_SOCKET, void *buf, size_t len, int,
	ACL_VSTREAM*, void *ctx)
{
#ifdef HAS_POLARSSL
	socket_stream* cli = (socket_stream*) ctx;
	int   ret;

	while ((ret = ::ssl_read((ssl_context*) cli->ssl_,
		(unsigned char*) buf, len)) < 0)
	{
		if (ret != POLARSSL_ERR_NET_WANT_READ
			&& ret != POLARSSL_ERR_NET_WANT_WRITE)
		{
			return ACL_VSTREAM_EOF;
		}
	}

	return ret;
#else
	(void) buf;
	(void) len;
	(void) ctx;
	return -1;
#endif
}

int socket_stream::ssl_send(ACL_SOCKET, const void *buf, size_t len,
	int, ACL_VSTREAM*, void *ctx)
{
#ifdef HAS_POLARSSL
	socket_stream* cli = (socket_stream*) ctx;
	int   ret;

	while ((ret = ::ssl_write((ssl_context*) cli->ssl_,
		(unsigned char*) buf, len)) < 0)
	{
		if (ret != POLARSSL_ERR_NET_WANT_READ
			&& ret != POLARSSL_ERR_NET_WANT_WRITE)
		{
			return ACL_VSTREAM_EOF;
		}
	}

	return ret;
#else
	(void) buf;
	(void) len;
	(void) ctx;
	return -1;
#endif
}

} // namespace acl
