#include "acl_stdafx.hpp"
#ifdef HAS_POLARSSL
# include "polarssl/ssl.h"
# include "polarssl/havege.h"
# include "polarssl/ctr_drbg.h"
# include "polarssl/entropy.h"
#endif
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stream/stream.hpp"
#include "acl_cpp/stream/polarssl_conf.hpp"
#include "acl_cpp/stream/polarssl_io.hpp"
#endif

namespace acl {

polarssl_io::polarssl_io(polarssl_conf& conf, bool server_side,
	bool non_block /* = false */)
: conf_(conf)
, server_side_(server_side)
, non_block_(non_block)
, handshake_ok_(false)
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

polarssl_io& polarssl_io::set_non_blocking(bool yes)
{
	// 此处仅设置非阻塞 IO 标志位，至于套接字是否被设置了非阻塞模式
	// 由应用自己来决定

	non_block_ = yes;
	return *this;
}

bool polarssl_io::open(ACL_VSTREAM* s)
{
	if (s == NULL)
	{
		logger_error("s null");
		return false;
	}

#ifdef HAS_POLARSSL
	// 防止重复调用 open 过程
	if (ssl_ != NULL)
	{
		// 如果是同一个流，则返回 true
		if (stream_ == s)
			return true;

		// 否则，禁止同一个 SSL IO 对象被绑在不同的流对象上
		logger_error("open again, stream_ changed!");
		return false;
	}

	stream_ = s;

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
	safe_snprintf(pers, sizeof(pers), "SSL Pthread Thread %lu",
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

	// 非阻塞模式下先不启动 SSL 握手过程
	if (non_block_)
		return true;

	// 阻塞模式下可以启动 SSL 握手过程
	return handshake();
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
	logger_error("HAS_POLARSSL not defined!");
#endif

	return true;
}

bool polarssl_io::handshake()
{
#ifdef HAS_POLARSSL
	if (handshake_ok_)
		return true;

	while (true)
	{
		// SSL 握手过程
		int ret = ::ssl_handshake((ssl_context*) ssl_);
		if (ret == 0)
		{
			handshake_ok_ = true;
			return true;
		}

		if (ret != POLARSSL_ERR_NET_WANT_READ
			&& ret != POLARSSL_ERR_NET_WANT_WRITE)
		{
			logger_error("ssl_handshake failed: -0x%04x", ret);
			return false;
		}

		if (non_block_)
			break;
	}

	return true;
#else
	logger_error("HAS_POLARSSL not defined!");
	return false;
#endif
}

bool polarssl_io::check_peer()
{
#ifdef HAS_POLARSSL
	int   ret = ssl_get_verify_result((ssl_context*) ssl_);
	if (ret != 0)
	{
		if (!ssl_get_peer_cert((ssl_context*) ssl_))
			logger("no client certificate sent");

		if ((ret & BADCERT_EXPIRED) != 0)
			logger("client certificate has expired");

		if ((ret & BADCERT_REVOKED) != 0)
			logger("client certificate has been revoked");

		if ((ret & BADCERT_NOT_TRUSTED) != 0)
			logger("self-signed or not signed by a trusted CA");

		return false;
	}
	else
		return true;
#else
	logger_error("HAS_POLARSSL not defined!");
	return false;
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
		if (non_block_)
			return ACL_VSTREAM_EOF;
	}

	// 如果 SSL 缓冲区中还有未读数据，则需要重置流可读标志位，
	// 这样可以触发 acl_vstream.c 及 events.c 中的系统读过程
	if (ssl_get_bytes_avail((ssl_context*) ssl_) > 0)
		stream_->read_ready = 1;
	// 否则，取消可读状态，表明 SSL 缓冲区里没有数据
	else
		stream_->read_ready = 0;

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
		if (non_block_)
			return ACL_VSTREAM_EOF;
	}

	return ret;
#else
	(void) buf;
	(void) len;
	logger_error("HAS_POLARSSL not defined!");
	return -1;
#endif
}

int polarssl_io::sock_read(void *ctx, unsigned char *buf, size_t len)
{
#ifdef HAS_POLARSSL
	polarssl_io* io = (polarssl_io*) ctx;
	ACL_VSTREAM* vs = io->stream_;
	ACL_SOCKET fd = ACL_VSTREAM_SOCK(vs);

	//logger(">>>non_block: %s, sys_ready: %s",
	//	io->non_block_ ? "yes" : "no",
	//	vs->read_ready ? "yes":"no");

	// 非阻塞模式下，如果 read_ready 标志位为 0，则说明有可能
	// 本次 IO 将读不到数据，为了防止该读过程被阻塞，所以此处直接
	// 返回给 polarssl 并告之等待下次读，下次读操作将由事件引擎触发，
	// 这样做的优点是在非阻塞模式下即使套接字没有设置为非阻塞状态
	// 也不会阻塞线程，但缺点是增加了事件循环触发的次数
	if (io->non_block_ && vs->read_ready == 0)
	{
		 int   ret = acl_readable(fd);
		 if (ret == -1)
			 return POLARSSL_ERR_NET_RECV_FAILED;
		 else if (ret == 0)
		 {
			// 必须在此处设置系统的 errno 号，此处是模拟了
			// 非阻塞读过程
			acl_set_error(ACL_EWOULDBLOCK);
			return POLARSSL_ERR_NET_WANT_READ;
		 }
		 // else: ret == 1
	}

	// acl_socket_read 内部会根据 vs->read_ready 标志位决定是否需要
	// 以超时方式读数据，同时会自动清除 vs->read_ready 标志位
	int ret = acl_socket_read(fd, buf, len, vs->rw_timeout, vs, NULL);

	if (ret < 0)
	{
		int   errnum = acl_last_error();

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
	logger_error("HAS_POLARSSL not defined!");
	return -1;
#endif
}

int polarssl_io::sock_send(void *ctx, const unsigned char *buf, size_t len)
{
#ifdef HAS_POLARSSL
	polarssl_io* io = (polarssl_io*) ctx;
	ACL_VSTREAM* vs = io->stream_;

	// 当为非阻塞模式时，超时等待为 0 秒
	int ret = acl_socket_write(ACL_VSTREAM_SOCK(vs), buf, len,
			io->non_block_ ? 0 : vs->rw_timeout, vs, NULL);
	if (ret < 0)
	{
		int   errnum = acl_last_error();

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
	logger_error("HAS_POLARSSL not defined!");
	return -1;
#endif
}

} // namespace acl
