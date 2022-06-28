#include "lib_acl.h"
#ifdef HAS_POLARSSL
# include "polarssl/ssl.h"
# include "polarssl/havege.h"
#endif
#include "acl_cpp/stdlib/log.hpp"
#include "ssl_aio_stream.hpp"

namespace acl
{

ssl_aio_stream::ssl_aio_stream(aio_handle* handle, ACL_ASTREAM* stream,
	bool opened /* = false */, bool use_ssl /* = true */)
: aio_stream(handle), aio_socket_stream(handle, stream, opened)
{
#ifdef HAS_POLARSSL
	if (use_ssl)
	{
		ssl_ = acl_mycalloc(1, sizeof(ssl_context));
		ssn_ = acl_mycalloc(1, sizeof(ssl_session));
		hs_  = acl_mymalloc(sizeof(havege_state));
	}
	else
	{
		ssl_ = NULL;
		ssn_ = NULL;
		hs_ = NULL;
	}
#else
	(void) use_ssl;
#endif
}

ssl_aio_stream::ssl_aio_stream(aio_handle* handle, ACL_SOCKET fd,
	bool use_ssl /* = true */)
: aio_stream(handle), aio_socket_stream(handle, fd)
{
#ifdef HAS_POLARSSL
	if (use_ssl)
	{
		ssl_ = acl_mycalloc(1, sizeof(ssl_context));
		ssn_ = acl_mycalloc(1, sizeof(ssl_session));
		hs_  = acl_mymalloc(sizeof(havege_state));
	}
	else
	{
		ssl_ = NULL;
		ssn_ = NULL;
		hs_ = NULL;
	}
#else
	(void) use_ssl;
#endif
}

ssl_aio_stream::~ssl_aio_stream()
{
	clear();
}

void ssl_aio_stream::clear()
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

bool ssl_aio_stream::open_callback()
{
#ifdef HAS_POLARSSL
	if (ssl_)
	{
		acl_assert(ssn_);
		acl_assert(hs_);
		return ssl_client_init();
	}
	return true;
#else
	return false;
#endif
}

ssl_aio_stream* ssl_aio_stream::open(aio_handle* handle,
	const char* addr, int timeout, bool use_ssl /* = true */)
{
	acl_assert(handle);

	ACL_ASTREAM* astream =
		acl_aio_connect(handle->get_handle(), addr, timeout);
	if (astream == NULL)
		return NULL;

	ssl_aio_stream* stream = new ssl_aio_stream(handle, astream, false);

	// 调用基类的 hook_error 以向 handle 中增加异步流计数,
	// 同时 hook 关闭及超时回调过程
	stream->hook_error();
	// hook 连接成功的回调过程
	stream->hook_open();

	// 如果是使用SSL，则注册本类对象为连接成功后的第一个回调过程
	if (use_ssl)
		stream->add_open_callback(stream);

	return stream;
}

bool ssl_aio_stream::open_ssl(bool on)
{
	ACL_VSTREAM* stream = get_vstream();
	acl_assert(stream);

	if (stream == NULL)
	{
		logger_error("stream null");
		return false;
	}

	if (on)
	{
#ifdef HAS_POLARSSL
		// 如果打开已经是 SSL 模式的流，则直接返回
		if (ssl_ != NULL)
		{
			acl_assert(ssn_);
			acl_assert(hs_);
			return true;
		}
#endif

		// 打开 SSL 流模式
		return ssl_client_init();
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
			return true;
		}
#endif

		// 清除与 SSL 相关的对象
		clear();

		// 切换成非 SSL 流模式
		acl_vstream_ctl(stream,
			ACL_VSTREAM_CTL_READ_FN, acl_socket_read,
			ACL_VSTREAM_CTL_WRITE_FN, acl_socket_write,
			ACL_VSTREAM_CTL_CTX, this,
			ACL_VSTREAM_CTL_END);
		return true;
	}
}

bool ssl_aio_stream::ssl_client_init()
{
#ifdef HAS_POLARSSL
	ACL_VSTREAM* stream = get_vstream();
	acl_assert(stream);

	// 0. Initialize the RNG and the session data
	havege_init((havege_state*) hs_);

	int   ret;
	if ((ret = ssl_init((ssl_context*) ssl_)) != 0)
	{
		logger_error("failed, ssl_init returned %d", ret);
		return false;
	}

	ssl_set_endpoint((ssl_context*) ssl_, SSL_IS_CLIENT);
	ssl_set_authmode((ssl_context*) ssl_, SSL_VERIFY_NONE);

	ssl_set_rng((ssl_context*) ssl_, ::havege_random, hs_);
	//ssl_set_dbg((ssl_context*) ssl_, my_debug, stdout);
	ssl_set_bio((ssl_context*) ssl_, __sock_read, this, __sock_send, this);

	const int* cipher_suites = ssl_list_ciphersuites();
	if (cipher_suites == NULL)
	{
		logger_error("ssl_list_ciphersuites null");
		return false;
	}

	ssl_set_ciphersuites((ssl_context*) ssl_, cipher_suites);
	ssl_set_session((ssl_context*) ssl_, (ssl_session*) ssn_);

	acl_vstream_ctl(stream,
		ACL_VSTREAM_CTL_READ_FN, __ssl_read,
		ACL_VSTREAM_CTL_WRITE_FN, __ssl_send,
		ACL_VSTREAM_CTL_CTX, this,
		ACL_VSTREAM_CTL_END);
	acl_tcp_set_nodelay(ACL_VSTREAM_SOCK(stream));
#endif
	return true;
}

int ssl_aio_stream::__sock_read(void *ctx, unsigned char *buf, size_t len)
{
#ifdef HAS_POLARSSL
	ssl_aio_stream* cli = (ssl_aio_stream*) ctx;
	ACL_VSTREAM* stream = cli->get_vstream();
	acl_assert(stream);
	int   ret, errnum;

	if ((ret = acl_socket_read(ACL_VSTREAM_SOCK(stream), buf, len,
		0, stream, NULL)) < 0)
	{
		errnum = acl_last_error();
		if (errnum == ACL_EINTR)
			return POLARSSL_ERR_NET_WANT_READ;
		else if (errnum == ACL_EWOULDBLOCK)
			return POLARSSL_ERR_NET_WANT_READ;
#if ACL_EWOULDBLOCK != ACL_EAGAIN
		else if (errnum == ACL_EAGAIN)
			return POLARSSL_ERR_NET_WANT_READ;
#endif
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

int ssl_aio_stream::__sock_send(void *ctx, const unsigned char *buf, size_t len)
{
#ifdef HAS_POLARSSL
	ssl_aio_stream* cli = (ssl_aio_stream*) ctx;
	ACL_VSTREAM* stream = cli->get_vstream();
	acl_assert(stream);
	int   ret, errnum;

	if ((ret = acl_socket_write(ACL_VSTREAM_SOCK(stream), buf, len,
		0, stream, NULL)) < 0)
	{
		errnum = acl_last_error();
		if (errnum == ACL_EINTR)
			return POLARSSL_ERR_NET_WANT_WRITE;

		else if (errnum == ACL_EWOULDBLOCK)
			return POLARSSL_ERR_NET_WANT_WRITE;
#if ACL_EWOULDBLOCK != ACL_EAGAIN
		else if (errnum == ACL_EAGAIN)
			return POLARSSL_ERR_NET_WANT_WRITE;
#endif
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

int ssl_aio_stream::__ssl_read(ACL_SOCKET, void *buf, size_t len,
	int, ACL_VSTREAM*, void *ctx)
{
#ifdef HAS_POLARSSL
	ssl_aio_stream* cli = (ssl_aio_stream*) ctx;
	int   ret;

	while ((ret = ::ssl_read((ssl_context*) cli->ssl_,
		(unsigned char*) buf, len)) <= 0)
	{
		if (ret == POLARSSL_ERR_NET_WANT_READ
			|| ret == POLARSSL_ERR_NET_WANT_WRITE)
		{
			if (acl_last_error() != ACL_EWOULDBLOCK)
				acl_set_error(ACL_EWOULDBLOCK);
		}
		return ACL_VSTREAM_EOF;
	}
	return ret;
#else
	(void) buf;
	(void) len;
	(void) ctx;
	return -1;
#endif
}

int ssl_aio_stream::__ssl_send(ACL_SOCKET, const void *buf, size_t len,
	int, ACL_VSTREAM*, void *ctx)
{
#ifdef HAS_POLARSSL
	ssl_aio_stream* cli = (ssl_aio_stream*) ctx;
	int   ret;

	while ((ret = ::ssl_write((ssl_context*) cli->ssl_,
		(unsigned char*) buf, len)) <= 0)
	{
		if (ret == POLARSSL_ERR_NET_WANT_READ
			|| ret == POLARSSL_ERR_NET_WANT_WRITE)
		{
			if (acl_last_error() != ACL_EWOULDBLOCK)
				acl_set_error(ACL_EWOULDBLOCK);
		}
		return ACL_VSTREAM_EOF;
	}
	return ret;
#else
	(void) buf;
	(void) len;
	(void) ctx;
	return -1;
#endif
}

}  // namespace acl
