#include "acl_stdafx.hpp"
#include "polarssl/ssl.h"
#include "polarssl/havege.h"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/ssl_stream.hpp"

//#ifndef HAS_POLARSSL
//#define HAS_POLARSSL
//#endif

namespace acl
{

ssl_stream::ssl_stream(void)
#ifdef HAS_POLARSSL
: ssl_(NULL)
, ssn_(NULL)
, hs_(NULL)
#endif
{

}

ssl_stream::~ssl_stream(void)
{
	clear();
}

void ssl_stream::clear(void)
{
#ifdef HAS_POLARSSL
	if (ssl_)
	{
		ssl_free(ssl_);
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

bool ssl_stream::open_ssl(ACL_SOCKET fd, bool use_ssl /* = true */)
{
	ACL_VSTREAM* conn = acl_vstream_fdopen(fd, O_RDWR,
		8192, 0, ACL_VSTREAM_TYPE_SOCK);
	acl_assert(conn);
	return (open_ssl(conn, use_ssl));
}

bool ssl_stream::open_ssl(const char* addr, int conn_timeout,
	int rw_timeout, bool use_ssl /* = true */)
{
	ACL_VSTREAM* conn = acl_vstream_connect(addr, ACL_BLOCKING,
		conn_timeout, rw_timeout, 8192);
	if (conn == NULL)
		return (false);
	return (open_ssl(conn, use_ssl));
}

bool ssl_stream::open_ssl(ACL_VSTREAM* vstream, bool use_ssl /* = true */)
{
	bool ret = open(vstream);
	acl_assert(ret);

	if (use_ssl && ssl_client_init() == false)
	{
		(void) this->close();
		return (false);
	}

	return (true);
}

bool ssl_stream::open_ssl(bool on)
{
	if (m_pStream == NULL)
	{
		logger_error("m_pStream null");
		return (false);
	}

	if (on)
	{
#ifdef HAS_POLARSSL
		// 如果打开已经是 SSL 模式的流，则直接返回
		if (ssl_ != NULL)
		{
			acl_assert(ssn_);
			acl_assert(hs_);
			return (true);
		}
#endif
			
		// 打开 SSL 流模式
		return (ssl_client_init());
	}
	else
	{
#ifdef HAS_POLARSSL
		// 如果关闭非 SSL 模式的流，则直接返回
		if (ssl_ == NULL)
		{
			ssl_ = NULL;
			acl_assert(ssn_ == NULL);
			acl_assert(hs_ == NULL);
			return (true);
		}
#endif

		// 清除与 SSL 相关的对象
		clear();

		// 切换成非 SSL 流模式
		acl_vstream_ctl(m_pStream,
			ACL_VSTREAM_CTL_READ_FN, acl_socket_read,
			ACL_VSTREAM_CTL_WRITE_FN, acl_socket_write,
			ACL_VSTREAM_CTL_CTX, this,
			ACL_VSTREAM_CTL_END);
		return (true);
	}
}

bool ssl_stream::ssl_client_init()
{
	acl_assert(m_pStream);

#ifdef HAS_POLARSSL
	ssl_ = (ssl_context*) acl_mycalloc(1, sizeof(ssl_context));
	ssn_ = (ssl_session*) acl_mycalloc(1, sizeof(ssl_session));
	hs_ = (havege_state*) acl_mymalloc(sizeof(havege_state));

	// 0. Initialize the RNG and the session data
	havege_init((havege_state*) hs_);

	int   ret;
	if ((ret = ssl_init(ssl_)) != 0)
	{
		logger_error("failed, ssl_init returned %d", ret);
		return (false);
	}

	ssl_set_endpoint(ssl_, SSL_IS_CLIENT);
	ssl_set_authmode(ssl_, SSL_VERIFY_NONE);

	ssl_set_rng(ssl_, havege_random, hs_);
	//ssl_set_dbg(ssl_, my_debug, stdout);
	ssl_set_bio(ssl_, __sock_read, this, __sock_send, this);

	ssl_set_ciphersuites(ssl_, ssl_default_ciphersuites);
	ssl_set_session(ssl_, 1, 600, ssn_);

	acl_vstream_ctl(m_pStream,
		ACL_VSTREAM_CTL_READ_FN, __ssl_read,
		ACL_VSTREAM_CTL_WRITE_FN, __ssl_send,
		ACL_VSTREAM_CTL_CTX, this,
		ACL_VSTREAM_CTL_END);
	acl_tcp_set_nodelay(ACL_VSTREAM_SOCK(m_pStream));
#endif
	return (true);
}

int ssl_stream::__sock_read(void *ctx, unsigned char *buf, size_t len)
{
#ifdef HAS_POLARSSL
	ssl_stream* cli = (ssl_stream*) ctx;
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
			return (POLARSSL_ERR_NET_WANT_READ);
		else if (errnum == ACL_ECONNRESET || errno == EPIPE)
			return (POLARSSL_ERR_NET_CONN_RESET);
		else
			return (POLARSSL_ERR_NET_RECV_FAILED);
	}

	return (ret);
#else
	(void) ctx;
	(void) buf;
	(void) len;
	return (-1);
#endif
}

int ssl_stream::__sock_send(void *ctx, const unsigned char *buf, size_t len)
{
#ifdef HAS_POLARSSL
	ssl_stream* cli = (ssl_stream*) ctx;
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
			return (POLARSSL_ERR_NET_WANT_WRITE);
		else if (errnum == ACL_ECONNRESET || errno == EPIPE)
			return (POLARSSL_ERR_NET_CONN_RESET);
		else
			return (POLARSSL_ERR_NET_SEND_FAILED);
	}

	return (ret);
#else
	(void) ctx;
	(void) buf;
	(void) len;
	return (-1);
#endif
}

int ssl_stream::__ssl_read(ACL_SOCKET, void *buf, size_t len, int,
	ACL_VSTREAM*, void *ctx)
{
#ifdef HAS_POLARSSL
	ssl_stream* cli = (ssl_stream*) ctx;
	int   ret;

	while ((ret = ::ssl_read(cli->ssl_, (unsigned char*) buf, len)) < 0)
	{
		if (ret != POLARSSL_ERR_NET_WANT_READ
			&& ret != POLARSSL_ERR_NET_WANT_WRITE)
		{
			return (ACL_VSTREAM_EOF);
		}
	}

	return (ret);
#else
	(void) buf;
	(void) len;
	(void) ctx;
	return (-1);
#endif
}

int ssl_stream::__ssl_send(ACL_SOCKET, const void *buf, size_t len,
	int, ACL_VSTREAM*, void *ctx)
{
#ifdef HAS_POLARSSL
	ssl_stream* cli = (ssl_stream*) ctx;
	int   ret;

	while ((ret = ::ssl_write(cli->ssl_, (unsigned char*) buf, len)) < 0)
	{
		if (ret != POLARSSL_ERR_NET_WANT_READ
			&& ret != POLARSSL_ERR_NET_WANT_WRITE)
		{
			return (ACL_VSTREAM_EOF);
		}
	}

	return (ret);
#else
	(void) buf;
	(void) len;
	(void) ctx;
	return (-1);
#endif
}

}  // namespace acl
