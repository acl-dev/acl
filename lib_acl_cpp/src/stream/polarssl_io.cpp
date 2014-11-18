#include "acl_stdafx.hpp"
#ifdef HAS_POLARSSL
# include "polarssl/ssl.h"
# include "polarssl/havege.h"
# include "polarssl/ctr_drbg.h"
# include "polarssl/entropy.h"
#endif
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stream/stream.hpp"
#include "acl_cpp/stream/polarssl_conf.hpp"
#include "acl_cpp/stream/polarssl_io.hpp"

namespace acl {

polarssl_io::polarssl_io(polarssl_conf& conf, bool server_side)
: conf_(conf)
, server_side_(server_side)
, ssl_(NULL)
, ssn_(NULL)
, rnd_(NULL)
, stream_(NULL)
{
}

polarssl_io::~polarssl_io()
{
#ifdef HAS_POLARSSL
	if (ssl_)
	{
		::ssl_free((ssl_context*) ssl_);
		acl_myfree(ssl_);
	}
	if (ssn_)
	{
		ssl_session_free((ssl_session*) ssn_);
		acl_myfree(ssn_);
	}

	// 使用 havege_random 随机数生成器时，在一些虚机上并不能保证随机性，
	// 建议使用 ctr_drbg_random 随机数生成器
# undef HAS_HAVEGE

# ifdef HAS_HAVEGE
	if (rnd_)
	{
//		::havege_free((havege_state*) rnd_);
		acl_myfree(rnd_);
	}
# else
	if (rnd_)
	{
#  ifdef POLARSSL_1_3_X
		ctr_drbg_free((ctr_drbg_context*) rnd_);
#  endif
		acl_myfree(rnd_);
	}
# endif

#endif
}

void polarssl_io::destroy()
{
	delete this;
}

#ifdef	DEBUG_SSL
static void my_debug( void *ctx, int level acl_unused, const char *str )
{
	fprintf((FILE *) ctx, "%s", str);
	fflush((FILE *) ctx);
}
#endif

bool polarssl_io::open(stream* s)
{
	if (s == NULL)
	{
		logger_error("stream null");
		return false;
	}

	stream_ = s;

#ifdef HAS_POLARSSL
	// 防止重复调用 open 过程
	acl_assert(ssl_ == NULL);

	ssl_ = acl_mycalloc(1, sizeof(ssl_context));

	int   ret;

	// 初始化 SSL 对象
	if ((ret = ::ssl_init((ssl_context*) ssl_)) != 0)
	{
		logger_error("failed, ssl_init error: -0x%04x\n", ret);
		acl_myfree(ssl_);
		ssl_ = NULL;
		return false;
	}

	// 需要区分 SSL 连接是客户端模式还是服务器模式
	if (server_side_)
		::ssl_set_endpoint((ssl_context*) ssl_, SSL_IS_SERVER);
	else
		::ssl_set_endpoint((ssl_context*) ssl_, SSL_IS_CLIENT);

	// 初始化随机数生成过程

# ifdef HAS_HAVEGE
	rnd_  = acl_mymalloc(sizeof(havege_state));
	::havege_init((havege_state*) rnd_);

	// 设置随机数生成器
	::ssl_set_rng((ssl_context*) ssl_, havege_random, rnd_);
# else
	rnd_ = acl_mymalloc(sizeof(ctr_drbg_context));

	char pers[50];
	snprintf(pers, sizeof(pers), "SSL Pthread Thread %lu",
		(unsigned long) acl_pthread_self());

	ret = ::ctr_drbg_init((ctr_drbg_context*) rnd_, entropy_func,
			(entropy_context*) conf_.get_entropy(),
			(const unsigned char *) pers, strlen(pers));
	if (ret != 0)
	{
		logger_error("ctr_drbg_init error: -0x%04x\n", ret);
		return false;
	}

	// 设置随机数生成器
	::ssl_set_rng((ssl_context*) ssl_, ctr_drbg_random, rnd_);
# endif

# ifdef	DEBUG_SSL
	ssl_set_dbg((ssl_context*) ssl_, my_debug, stdout);
# endif
	
	if (!server_side_)
	{
		// 只有客户端模式下才会调用此过程

		ssn_ = acl_mycalloc(1, sizeof(ssl_session));
# ifdef POLARSSL_1_3_X
		ret = ::ssl_set_session((ssl_context*) ssl_,
			(ssl_session*) ssn_);
		if (ret != 0)
		{
			logger_error("ssl_set_session error: -0x%04x\n", ret);
			acl_myfree(ssn_);
			ssn_ = NULL;
		}
# else
		::ssl_set_session((ssl_context*) ssl_, (ssl_session*) ssn_);
# endif
	}

	// 配置全局参数（包含证书、私钥）
	conf_.setup_certs(ssl_, server_side_);

	// Setup SSL IO callback
	::ssl_set_bio((ssl_context*) ssl_, sock_read, this, sock_send, this);

	// SSL 握手过程
	while((ret = ::ssl_handshake((ssl_context*) ssl_)) != 0)
	{
		if (ret != POLARSSL_ERR_NET_WANT_READ
			&& ret != POLARSSL_ERR_NET_WANT_WRITE)
		{
			logger_error("ssl_handshake failed: -0x%04x", ret);
			return false;
		}
	}

#if 0
	if ((ret = ssl_get_verify_result((ssl_context*) ssl_)) != 0)
	{
		printf("failed\n");

		if (!ssl_get_peer_cert((ssl_context*) ssl_))
			printf("! no client certificate sent\n");

		if ((ret & BADCERT_EXPIRED) != 0)
			printf("! client certificate has expired\n");

		if ((ret & BADCERT_REVOKED) != 0)
			printf("! client certificate has been revoked\n");

		if ((ret & BADCERT_NOT_TRUSTED) != 0)
			printf("! self-signed or not signed by a trusted CA\n");

		printf("\n");
	}
#endif

	return true;
#else
	logger_error("define HAS_POLARSSL first!");
	return false;
#endif
}

bool polarssl_io::on_close(bool alive)
{
#ifdef HAS_POLARSSL
	if (ssl_ == NULL)
	{
		logger_error("ssl_ null");
		return false;
	}
	if (stream_ == NULL)
	{
		logger_error("stream_ null");
		return false;
	}

	if (!alive)
		return false;

	int   ret;
	while((ret = ssl_close_notify((ssl_context*) ssl_ )) < 0)
	{
		if( ret != POLARSSL_ERR_NET_WANT_READ &&
			ret != POLARSSL_ERR_NET_WANT_WRITE )
		{
			logger_warn("ssl_close_notify error: -0x%04x", ret);
			return false;
		}
	}
#else
	(void) alive;
#endif

	return true;
}

int polarssl_io::sock_read(void *ctx, unsigned char *buf, size_t len)
{
#ifdef HAS_POLARSSL
	polarssl_io* io = (polarssl_io*) ctx;
	int   ret, timeout = 120;
	ACL_VSTREAM* vs = io->stream_->get_vstream();
	ACL_SOCKET fd = ACL_VSTREAM_SOCK(vs);

	ret = acl_socket_read(fd, buf, len, timeout, vs, NULL);
	if (ret < 0)
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
	logger_error("HAS_POLARSSL not defined!");
	return -1;
#endif
}

int polarssl_io::sock_send(void *ctx, const unsigned char *buf, size_t len)
{
#ifdef HAS_POLARSSL
	polarssl_io* io = (polarssl_io*) ctx;
	int   ret, timeout = 120;
	ACL_VSTREAM* vs = io->stream_->get_vstream();
	ACL_SOCKET fd = ACL_VSTREAM_SOCK(vs);

	ret = acl_socket_write(fd, buf, len, timeout, vs, NULL);
	if (ret < 0)
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
		{
			logger_error("write error: %s", last_serror());
			return POLARSSL_ERR_NET_SEND_FAILED;
		}
	}

	return ret;
#else
	(void) ctx;
	(void) buf;
	(void) len;
	logger_error("HAS_POLARSSL not defined!");
	return -1;
#endif
}

int polarssl_io::read(void* buf, size_t len)
{
#ifdef HAS_POLARSSL
	int   ret;

	while ((ret = ::ssl_read((ssl_context*) ssl_,
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
	logger_error("HAS_POLARSSL not defined!");
	return -1;
#endif
}

int polarssl_io::send(const void* buf, size_t len)
{
#ifdef HAS_POLARSSL
	int   ret;

	while ((ret = ::ssl_write((ssl_context*) ssl_,
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
	logger_error("HAS_POLARSSL not defined!");
	return -1;
#endif
}

} // namespace acl
