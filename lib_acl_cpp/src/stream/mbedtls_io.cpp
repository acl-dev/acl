#include "acl_stdafx.hpp"

#ifdef HAS_MBEDTLS_DLL
# ifndef HAS_MBEDTLS
#  define HAS_MBEDTLS
# endif
#endif

#ifdef HAS_MBEDTLS
# include "mbedtls/ssl.h"
# include "mbedtls/havege.h"
# include "mbedtls/ctr_drbg.h"
# include "mbedtls/entropy.h"
# include "mbedtls/net_sockets.h"
#endif
 
#ifndef ACL_PREPARE_COMPILE
# include "acl_cpp/stdlib/snprintf.hpp"
# include "acl_cpp/stdlib/log.hpp"
# include "acl_cpp/stdlib/util.hpp"
# include "acl_cpp/stream/stream.hpp"
# include "acl_cpp/stream/mbedtls_conf.hpp"
# include "acl_cpp/stream/mbedtls_io.hpp"
#endif

#if defined(HAS_MBEDTLS_DLL)

# define CTR_DRBG_FREE_NAME		"mbedtls_ctr_drbg_free"

# ifdef HAS_HAVEGE
#  define HAVEGE_INIT_NAME		"mbedtls_havege_init"
#  define HAVEGE_RANDOM_NAME		"mbedtls_havege_random"
# else
#  define CTR_DRBG_INIT_NAME		"mbedtls_ctr_drbg_init"
#  define ENTROPY_FUNC_NAME		"mbedtls_entropy_func"
#  define CTR_DRBG_RANDOM_NAME		"mbedtls_ctr_drbg_random"
# endif

# ifdef DEBUG_SSL
#  define SSL_SET_DBG_NAME		"mbedtls_ssl_set_dbg"
# endif

# define SSL_SET_SESSION_NAME		"mbedtls_ssl_set_session"
# define SSL_SET_RNG_NAME		"mbedtls_ssl_set_rng"
# define SSL_INIT_NAME			"mbedtls_ssl_init"
# define SSL_FREE_NAME			"mbedtls_ssl_free"
# define SSL_SET_ENDPOINT_NAME		"mbedtls_ssl_set_endpoint"
# define SSL_SESSION_FREE_NAME		"mbedtls_ssl_session_free"
# define SSL_SET_BIO_NAME		"mbedtls_ssl_set_bio"
# define SSL_CLOSE_NOTIFY_NAME		"mbedtls_ssl_close_notify"
# define SSL_HANDSHAKE_NAME		"mbedtls_ssl_handshake"
# define SSL_GET_VERIFY_RESULT_NAME	"mbedtls_ssl_get_verify_result"
# define SSL_GET_PEER_CERT_NAME		"mbedtls_ssl_get_peer_cert"
# define SSL_READ_NAME			"mbedtls_ssl_read"
# define SSL_WRITE_NAME			"mbedtls_ssl_write"
# define SSL_GET_BYTES_AVAIL_NAME	"mbedtls_ssl_get_bytes_avail"

typedef void (*ctr_drbg_free_fn)(mbedtls_ctr_drbg_context*);

# ifdef HAS_HAVEGE
typedef void (*havege_init_fn)(mbedtls_havege_state*);
typedef int (*havege_random_fn)(void*, unsigned char*, size_t);
# else
typedef int (*ctr_drbg_init_fn)(mbedtls_ctr_drbg_context*,
		int (*)(void*, unsigned char*, size_t), void *,
		const unsigned char*, size_t);
typedef int (*ctr_drbg_random_fn)(void*, unsigned char*, size_t);
typedef int (*entropy_func_fn)(void*, unsigned char*, size_t);
# endif

# ifdef DEBUG_SSL
typedef void (ssl_set_dbg_fn)(mbedtls_ssl_context*,
		void (*)(void*, int, const char*), void*);
# endif

typedef int  (*ssl_set_session_fn)(mbedtls_ssl_context*, const mbedtls_ssl_session*);
typedef void (ssl_set_session_cache_fn)(mbedtls_ssl_context*,
		int (*)(void*, mbedtls_ssl_session*), void*,
		int (*)(void*, const mbedtls_ssl_session*), void*);
typedef void (*ssl_set_rng_fn)(mbedtls_ssl_context*,
		int (*f_rng)(void*, unsigned char*, size_t), void *p_rng);
typedef int (*ssl_init_fn)(mbedtls_ssl_context*);
typedef void (*ssl_free_fn)(mbedtls_ssl_context*);
typedef void (*ssl_set_endpoint_fn)(mbedtls_ssl_context*, int);
typedef void (*ssl_session_free_fn)(mbedtls_ssl_session*);
typedef void (*ssl_set_bio_fn)(mbedtls_ssl_context*,
		int (*)(void*, unsigned char*, size_t), void*,
		int (*)(void*, const unsigned char*, size_t), void*);
typedef int (*ssl_close_notify_fn)(mbedtls_ssl_context*);
typedef int (*ssl_handshake_fn)(mbedtls_ssl_context*);
typedef int (*ssl_get_verify_result_fn)(const mbedtls_ssl_context*);
typedef const mbedtls_x509_crt *(*ssl_get_peer_cert_fn)(const mbedtls_ssl_context*);
typedef int (*ssl_read_fn)(mbedtls_ssl_context*, unsigned char*, size_t);
typedef int (*ssl_write_fn)(mbedtls_ssl_context*, const unsigned char*, size_t);
typedef size_t (*ssl_get_bytes_avail_fn)(const mbedtls_ssl_context*);

static ctr_drbg_free_fn			__ctr_drbg_free;

# ifdef HAS_HAVEGE
static havege_init_fn			__havege_init;
static havege_random_fn			__havege_random;
# else
static ctr_drbg_init_fn			__ctr_drbg_init;
static ctr_drbg_random_fn		__ctr_drbg_random;
static entropy_func_fn			__entropy_func;
# endif
# ifdef DEBUG_SSL
static ssl_set_dbg_fn			__ssl_set_dbg;
# endif

static ssl_set_session_fn		__ssl_set_session;
//static ssl_set_session_cache_fn		__ssl_set_session_cache;
static ssl_set_rng_fn			__ssl_set_rng;
static ssl_init_fn			__ssl_init;
static ssl_free_fn			__ssl_free;
static ssl_set_endpoint_fn		__ssl_set_endpoint;
static ssl_session_free_fn		__ssl_session_free;
static ssl_set_bio_fn			__ssl_set_bio;
static ssl_close_notify_fn		__ssl_close_notify;
static ssl_handshake_fn			__ssl_handshake;
static ssl_get_verify_result_fn		__ssl_get_verify_result;
static ssl_get_peer_cert_fn		__ssl_get_peer_cert;
static ssl_read_fn			__ssl_read;
static ssl_write_fn			__ssl_write;
static ssl_get_bytes_avail_fn		__ssl_get_bytes_avail;

extern ACL_DLL_HANDLE __mbedtls_dll;  // defined in mbedtls_conf.cpp

void mbedtls_dll_load_io(void)
{
#define LOAD(name, type, fn) do {					\
	(fn) = (type) acl_dlsym(__mbedtls_dll, (name));		\
	if ((fn) == NULL)						\
		logger_fatal("dlsym %s error %s", name, acl_dlerror());	\
} while (0)

	acl_assert(__mbedtls_dll);

	LOAD(CTR_DRBG_FREE_NAME, ctr_drbg_free_fn, __ctr_drbg_free);

# ifdef HAS_HAVEGE
	LOAD(HAVEGE_INIT_NAME, havege_init_fn, __havege_init);
	LOAD(HAVEGE_RANDOM_NAME, havege_random_fn, __havege_random);
# else
	LOAD(CTR_DRBG_INIT_NAME, ctr_drbg_init_fn, __ctr_drbg_init);
	LOAD(CTR_DRBG_RANDOM_NAME, ctr_drbg_random_fn, __ctr_drbg_random);
	LOAD(ENTROPY_FUNC_NAME, entropy_func_fn, __entropy_func);
# endif

# ifdef DEBUG_SSL
	LOAD(SSL_SET_DBG_NAME, ssl_set_dbg_fn, __ssl_set_dbg);
# endif

	LOAD(SSL_SET_SESSION_NAME, ssl_set_session_fn, __ssl_set_session);
	LOAD(SSL_SET_RNG_NAME, ssl_set_rng_fn, __ssl_set_rng);
	LOAD(SSL_INIT_NAME, ssl_init_fn, __ssl_init);
	LOAD(SSL_FREE_NAME, ssl_free_fn, __ssl_free);
	LOAD(SSL_SET_ENDPOINT_NAME, ssl_set_endpoint_fn, __ssl_set_endpoint);
	LOAD(SSL_SESSION_FREE_NAME, ssl_session_free_fn, __ssl_session_free);
	LOAD(SSL_SET_BIO_NAME, ssl_set_bio_fn, __ssl_set_bio);
	LOAD(SSL_CLOSE_NOTIFY_NAME, ssl_close_notify_fn, __ssl_close_notify);
	LOAD(SSL_HANDSHAKE_NAME, ssl_handshake_fn, __ssl_handshake);
	LOAD(SSL_GET_VERIFY_RESULT_NAME, ssl_get_verify_result_fn, __ssl_get_verify_result);
	LOAD(SSL_GET_PEER_CERT_NAME, ssl_get_peer_cert_fn, __ssl_get_peer_cert);
	LOAD(SSL_READ_NAME, ssl_read_fn, __ssl_read);
	LOAD(SSL_WRITE_NAME, ssl_write_fn, __ssl_write);
	LOAD(SSL_GET_BYTES_AVAIL_NAME, ssl_get_bytes_avail_fn, __ssl_get_bytes_avail);
}

#elif defined(HAS_MBEDTLS)

# define __ctr_drbg_free		::mbedtls_ctr_drbg_free

# ifdef HAS_HAVEGE
#  define __havege_init			::mbedtls_havege_init
#  define __havege_random		::mbedtls_havege_random
# else
#  define __ctr_drbg_init		::mbedtls_ctr_drbg_init
#  define __ctr_drbg_random		::mbedtls_ctr_drbg_random
#  define __entropy_func		::mbedtls_entropy_func
# endif
# ifdef DEBUG_SSL
#  define __ssl_set_dbg			::mbedtls_ssl_set_dbg
# endif

# define __ssl_set_session		::mbedtls_ssl_set_session
//# define __ssl_set_session_cache	::mbedtls_ssl_set_session_cache
# define __ssl_set_rng			::mbedtls_ssl_set_rng
# define __ssl_init			::mbedtls_ssl_init
# define __ssl_free			::mbedtls_ssl_free
# define __ssl_set_endpoint		::mbedtls_ssl_set_endpoint
# define __ssl_session_free		::mbedtls_ssl_session_free
# define __ssl_set_bio			::mbedtls_ssl_set_bio
# define __ssl_close_notify		::mbedtls_ssl_close_notify
# define __ssl_handshake		::mbedtls_ssl_handshake
# define __ssl_get_verify_result	::mbedtls_ssl_get_verify_result
# define __ssl_get_peer_cert		::mbedtls_ssl_get_peer_cert
# define __ssl_read			::mbedtls_ssl_read
# define __ssl_write			::mbedtls_ssl_write
# define __ssl_get_bytes_avail		::mbedtls_ssl_get_bytes_avail

#endif

namespace acl {

mbedtls_io::mbedtls_io(mbedtls_conf& conf, bool server_side,
	bool nblock /* = false */)
: sslbase_io(conf, server_side, nblock)
, conf_(conf)
, ssl_(NULL)
, ssn_(NULL)
, rnd_(NULL)
{
#ifdef HAS_MBEDTLS
	conf.init_once();
#endif
}

mbedtls_io::~mbedtls_io(void)
{
#ifdef HAS_MBEDTLS
	if (ssl_) {
		__ssl_free((mbedtls_ssl_context*) ssl_);
		acl_myfree(ssl_);
		ssl_ = NULL;
	}
	if (ssn_) {
		__ssl_session_free((mbedtls_ssl_session*) ssn_);
		acl_myfree(ssn_);
		ssn_ = NULL;
	}

	// 使用 havege_random 随机数生成器时，在一些虚机上并不能保证随机性，
	// 建议使用 ctr_drbg_random 随机数生成器
# undef HAS_HAVEGE

# ifdef HAS_HAVEGE
	if (rnd_) {
		acl_myfree(rnd_);
		rnd_ = NULL;
	}
# else
	if (rnd_) {
		__ctr_drbg_free((mbedtls_ctr_drbg_context*) rnd_);
		rnd_ = NULL;
	}
# endif

#endif
}

void mbedtls_io::destroy(void)
{
	if (--(*refers_) == 0) {
		delete this;
	}
}

#ifdef	DEBUG_SSL
static void my_debug( void *ctx, int level acl_unused, const char *str )
{
	fprintf((FILE *) ctx, "%s", str);
	fflush((FILE *) ctx);
}
#endif

bool mbedtls_io::open(ACL_VSTREAM* s)
{
	if (s == NULL) {
		logger_error("s null");
		return false;
	}

#ifdef HAS_MBEDTLS
	// 防止重复调用 open 过程
	if (ssl_ != NULL) {
		// 如果是同一个流，则返回 true
		if (stream_ == s) {
			return true;
		} else if (ACL_VSTREAM_SOCK(stream_) == ACL_VSTREAM_SOCK(s)) {
			long long n = ++(*refers_);
			logger_warn("used by multiple stream, refers=%lld", n);
			return true;
		}

		// 否则，禁止同一个 SSL IO 对象被绑在不同的流对象上
		logger_error("open again, stream_ changed!");
		return false;
	}

	stream_ = s;
	++(*refers_);

	ssl_ = acl_mycalloc(1, sizeof(mbedtls_ssl_context));

	int ret;

	// 初始化 SSL 对象
	if ((ret = __ssl_init((mbedtls_ssl_context*) ssl_)) != 0) {
		logger_error("failed, ssl_init error: -0x%04x\n", ret);
		acl_myfree(ssl_);
		ssl_ = NULL;
		return false;
	}

	// 需要区分 SSL 连接是客户端模式还是服务器模式
	if (server_side_) {
		__ssl_set_endpoint((mbedtls_ssl_context*) ssl_, MBEDTLS_SSL_IS_SERVER);
	} else {
		__ssl_set_endpoint((mbedtls_ssl_context*) ssl_, MBEDTLS_SSL_IS_CLIENT);
	}

	// 初始化随机数生成过程

# ifdef HAS_HAVEGE
	rnd_ = acl_mymalloc(sizeof(mbedtls_havege_state));
	__havege_init((mbedtls_havege_state*) rnd_);

	// 设置随机数生成器
	__ssl_set_rng((mbedtls_ssl_context*) ssl_, __havege_random, rnd_);
# else
	rnd_ = acl_mymalloc(sizeof(mbedtls_ctr_drbg_context));

	char pers[50];
	safe_snprintf(pers, sizeof(pers), "SSL Pthread Thread %lu",
		(unsigned long) acl_pthread_self());

	ret = __ctr_drbg_init((mbedtls_ctr_drbg_context*) rnd_, __entropy_func,
			(mbedtls_entropy_context*) conf_.get_entropy(),
			(const unsigned char *) pers, strlen(pers));
	if (ret != 0) {
		logger_error("ctr_drbg_init error: -0x%04x\n", ret);
		return false;
	}

	// 设置随机数生成器
	__ssl_set_rng((mbedtls_ssl_context*) ssl_, __ctr_drbg_random, rnd_);
# endif

# ifdef	DEBUG_SSL
	__ssl_set_dbg((mbedtls_ssl_context*) ssl_, my_debug, stdout);
# endif
	
	if (!server_side_) {
		// 只有客户端模式下才会调用此过程

		ssn_ = acl_mycalloc(1, sizeof(mbedtls_ssl_session));
		ret = __ssl_set_session((mbedtls_ssl_context*) ssl_,
			(mbedtls_ssl_session*) ssn_);
		if (ret != 0) {
			logger_error("ssl_set_session error: -0x%04x\n", ret);
			acl_myfree(ssn_);
			ssn_ = NULL;
		}
	}

	// 配置全局参数（包含证书、私钥）
	conf_.setup_certs(ssl_, server_side_);

	// Setup SSL IO callback
	__ssl_set_bio((mbedtls_ssl_context*) ssl_, sock_read, this, sock_send, this);

	// 非阻塞模式下先不启动 SSL 握手过程
	if (nblock_) {
		return true;
	}

	// 阻塞模式下可以启动 SSL 握手过程
	return handshake();
#else
	logger_error("define HAS_MBEDTLS first!");
	return false;
#endif
}

bool mbedtls_io::on_close(bool alive)
{
#ifdef HAS_MBEDTLS
	if (ssl_ == NULL) {
		logger_error("ssl_ null");
		return false;
	}
	if (stream_ == NULL) {
		logger_error("stream_ null");
		return false;
	}

	if (!alive) {
		return false;
	}

	int   ret;
	while((ret = __ssl_close_notify((mbedtls_ssl_context*) ssl_ )) < 0) {
		if( ret != MBEDTLS_ERR_SSL_WANT_READ &&
			ret != MBEDTLS_ERR_SSL_WANT_WRITE ) {

			logger_warn("ssl_close_notify error: -0x%04x", ret);
			return false;
		}
	}
#else
	(void) alive;
	logger_error("HAS_MBEDTLS not defined!");
#endif

	return true;
}

bool mbedtls_io::handshake(void)
{
#ifdef HAS_MBEDTLS
	if (handshake_ok_) {
		return true;
	}

	while (true) {
		// SSL 握手过程
		int ret = __ssl_handshake((mbedtls_ssl_context*) ssl_);
		if (ret == 0) {
			handshake_ok_ = true;
			return true;
		}

		if (ret != MBEDTLS_ERR_SSL_WANT_READ
			&& ret != MBEDTLS_ERR_SSL_WANT_WRITE) {

			logger_error("ssl_handshake failed: -0x%04x", ret);
			return false;
		}

		if (nblock_) {
			break;
		}
	}

	return true;
#else
	logger_error("HAS_MBEDTLS not defined!");
	return false;
#endif
}

bool mbedtls_io::check_peer(void)
{
#ifdef HAS_MBEDTLS
	int   ret = __ssl_get_verify_result((mbedtls_ssl_context*) ssl_);
	if (ret != 0) {
		if (!__ssl_get_peer_cert((mbedtls_ssl_context*) ssl_)) {
			logger("no client certificate sent");
		}

		if ((ret & MBEDTLS_X509_BADCERT_EXPIRED) != 0) {
			logger("client certificate has expired");
		}

		if ((ret & MBEDTLS_X509_BADCERT_REVOKED) != 0) {
			logger("client certificate has been revoked");
		}

		if ((ret & MBEDTLS_X509_BADCERT_NOT_TRUSTED) != 0) {
			logger("self-signed or not signed by a trusted CA");
		}

		return false;
	} else {
		return true;
	}
#else
	logger_error("HAS_MBEDTLS not defined!");
	return false;
#endif
}

int mbedtls_io::read(void* buf, size_t len)
{
#ifdef HAS_MBEDTLS
	int   ret;

	while ((ret = __ssl_read((mbedtls_ssl_context*) ssl_,
		(unsigned char*) buf, len)) < 0) {

		if (ret != MBEDTLS_ERR_SSL_WANT_READ
			&& ret != MBEDTLS_ERR_SSL_WANT_WRITE) {

			return ACL_VSTREAM_EOF;
		}
		if (nblock_) {
			return ACL_VSTREAM_EOF;
		}
	}

	// 如果 SSL 缓冲区中还有未读数据，则需要重置流可读标志位，
	// 这样可以触发 acl_vstream.c 及 events.c 中的系统读过程
	if (__ssl_get_bytes_avail((mbedtls_ssl_context*) ssl_) > 0) {
		stream_->read_ready = 1;
	}
	// 否则，取消可读状态，表明 SSL 缓冲区里没有数据
	else {
		stream_->read_ready = 0;
	}

	return ret;
#else
	(void) buf;
	(void) len;
	logger_error("HAS_MBEDTLS not defined!");
	return -1;
#endif
}

int mbedtls_io::send(const void* buf, size_t len)
{
#ifdef HAS_MBEDTLS
	int   ret;

	while ((ret = __ssl_write((mbedtls_ssl_context*) ssl_,
		(unsigned char*) buf, len)) < 0) {

		if (ret != MBEDTLS_ERR_SSL_WANT_READ
			&& ret != MBEDTLS_ERR_SSL_WANT_WRITE) {

			return ACL_VSTREAM_EOF;
		}
		if (nblock_) {
			return ACL_VSTREAM_EOF;
		}
	}

	return ret;
#else
	(void) buf;
	(void) len;
	logger_error("HAS_MBEDTLS not defined!");
	return -1;
#endif
}

int mbedtls_io::sock_read(void *ctx, unsigned char *buf, size_t len)
{
#ifdef HAS_MBEDTLS
	mbedtls_io* io = (mbedtls_io*) ctx;
	ACL_VSTREAM* vs = io->stream_;
	ACL_SOCKET fd = ACL_VSTREAM_SOCK(vs);

	//logger(">>>non_block: %s, sys_ready: %s",
	//	io->nblock_ ? "yes" : "no",
	//	vs->read_ready ? "yes":"no");

	// 非阻塞模式下，如果 read_ready 标志位为 0，则说明有可能
	// 本次 IO 将读不到数据，为了防止该读过程被阻塞，所以此处直接
	// 返回给 mbedtls 并告之等待下次读，下次读操作将由事件引擎触发，
	// 这样做的优点是在非阻塞模式下即使套接字没有设置为非阻塞状态
	// 也不会阻塞线程，但缺点是增加了事件循环触发的次数
	if (io->nblock_ && vs->read_ready == 0) {
		 int ret = acl_readable(fd);
		 if (ret == -1) {
			 return MBEDTLS_ERR_NET_RECV_FAILED;
		 } else if (ret == 0) {
			// 必须在此处设置系统的 errno 号，此处是模拟了
			// 非阻塞读过程
			acl_set_error(ACL_EWOULDBLOCK);
			return MBEDTLS_ERR_SSL_WANT_READ;
		 }
		 // else: ret == 1
	}

	// acl_socket_read 内部会根据 vs->read_ready 标志位决定是否需要
	// 以超时方式读数据，同时会自动清除 vs->read_ready 标志位
	int ret = acl_socket_read(fd, buf, len, vs->rw_timeout, vs, NULL);

	if (ret < 0) {
		int errnum = acl_last_error();

		if (errnum == ACL_EINTR) {
			return MBEDTLS_ERR_SSL_WANT_READ;
		} else if (errnum == ACL_EWOULDBLOCK) {
			return MBEDTLS_ERR_SSL_WANT_READ;
#if ACL_EWOULDBLOCK != ACL_EAGAIN
		} else if (errnum == ACL_EAGAIN) {
			return MBEDTLS_ERR_SSL_WANT_READ;
#endif
		} else if (errnum == ACL_ECONNRESET || errno == EPIPE) {
			return MBEDTLS_ERR_NET_CONN_RESET;
		} else {
			return MBEDTLS_ERR_NET_RECV_FAILED;
		}
	}

	return ret;
#else
	(void) ctx;
	(void) buf;
	(void) len;
	logger_error("HAS_MBEDTLS not defined!");
	return -1;
#endif
}

int mbedtls_io::sock_send(void *ctx, const unsigned char *buf, size_t len)
{
#ifdef HAS_MBEDTLS
	mbedtls_io* io = (mbedtls_io*) ctx;
	ACL_VSTREAM* vs = io->stream_;

	// 当为非阻塞模式时，超时等待为 0 秒
	int ret = acl_socket_write(ACL_VSTREAM_SOCK(vs), buf, len,
			io->nblock_ ? 0 : vs->rw_timeout, vs, NULL);
	if (ret < 0) {
		int errnum = acl_last_error();

		if (errnum == ACL_EINTR) {
			return MBEDTLS_ERR_SSL_WANT_WRITE;
		} else if (errnum == ACL_EWOULDBLOCK) {
			return MBEDTLS_ERR_SSL_WANT_WRITE;
#if ACL_EWOULDBLOCK != ACL_EAGAIN
		} else if (errnum == ACL_EAGAIN) {
			return MBEDTLS_ERR_SSL_WANT_WRITE;
#endif
		} else if (errnum == ACL_ECONNRESET || errno == EPIPE) {
			return MBEDTLS_ERR_NET_CONN_RESET;
		} else {
			return MBEDTLS_ERR_NET_SEND_FAILED;
		}
	}

	return ret;
#else
	(void) ctx;
	(void) buf;
	(void) len;
	logger_error("HAS_MBEDTLS not defined!");
	return -1;
#endif
}

} // namespace acl
