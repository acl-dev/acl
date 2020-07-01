#include "acl_stdafx.hpp"

#ifdef HAS_MBEDTLS_DLL
# ifndef HAS_MBEDTLS
#  define HAS_MBEDTLS
# endif
#endif

#ifdef HAS_MBEDTLS
# include "mbedtls-2.7.12/ssl.h"
# include "mbedtls-2.7.12/havege.h"
# include "mbedtls-2.7.12/ctr_drbg.h"
# include "mbedtls-2.7.12/entropy.h"
# include "mbedtls-2.7.12/net_sockets.h"
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

# define SSL_INIT_NAME			"mbedtls_ssl_init"
# define SSL_FREE_NAME			"mbedtls_ssl_free"
# define SSL_SET_HOSTNAME_NAME		"mbedtls_ssl_set_hostname"
# define SSL_SET_BIO_NAME		"mbedtls_ssl_set_bio"
# define SSL_READ_NAME			"mbedtls_ssl_read"
# define SSL_WRITE_NAME			"mbedtls_ssl_write"
# define SSL_HANDSHAKE_NAME		"mbedtls_ssl_handshake"
# define SSL_SET_SESSION_NAME		"mbedtls_ssl_set_session"
# define SSL_SESSION_FREE_NAME		"mbedtls_ssl_session_free"
# define SSL_SESSION_RESET_NAME		"mbedtls_ssl_session_reset"
# define SSL_CLOSE_NOTIFY_NAME		"mbedtls_ssl_close_notify"
# define SSL_GET_VERIFY_RESULT_NAME	"mbedtls_ssl_get_verify_result"
# define SSL_GET_PEER_CERT_NAME		"mbedtls_ssl_get_peer_cert"
# define SSL_GET_BYTES_AVAIL_NAME	"mbedtls_ssl_get_bytes_avail"

typedef void (*ssl_init_fn)(mbedtls_ssl_context*);
typedef void (*ssl_free_fn)(mbedtls_ssl_context*);
typedef int  (*ssl_set_hostname_fn)(mbedtls_ssl_context*, const char*);
typedef void (*ssl_set_bio_fn)(mbedtls_ssl_context*, void*,
		int (*f_send)(void*, const unsigned char*, size_t),
		int (*f_recv)(void*, unsigned char*, size_t),
		int (*f_recv_timeo)(void*, unsigned char*, size_t, unsigned));
typedef int (*ssl_read_fn)(mbedtls_ssl_context*, unsigned char*, size_t);
typedef int (*ssl_write_fn)(mbedtls_ssl_context*, const unsigned char*, size_t);
typedef int  (*ssl_handshake_fn)(mbedtls_ssl_context*);
typedef int  (*ssl_set_session_fn)(mbedtls_ssl_context*, const mbedtls_ssl_session*);
typedef void (*ssl_session_free_fn)(mbedtls_ssl_session*);
typedef int  (*ssl_session_reset_fn)(mbedtls_ssl_context*);
typedef int  (*ssl_close_notify_fn)(mbedtls_ssl_context*);
typedef unsigned (*ssl_get_verify_result_fn)(const mbedtls_ssl_context*);
typedef const mbedtls_x509_crt *(*ssl_get_peer_cert_fn)(const mbedtls_ssl_context*);
typedef size_t (*ssl_get_bytes_avail_fn)(const mbedtls_ssl_context*);

static ssl_init_fn			__ssl_init;
static ssl_free_fn			__ssl_free;
static ssl_set_hostname_fn		__ssl_set_hostname;
static ssl_set_bio_fn			__ssl_set_bio;
static ssl_read_fn			__ssl_read;
static ssl_write_fn			__ssl_write;
static ssl_handshake_fn			__ssl_handshake;
static ssl_set_session_fn		__ssl_set_session;
static ssl_session_free_fn		__ssl_session_free;
static ssl_session_reset_fn		__ssl_session_reset;
static ssl_close_notify_fn		__ssl_close_notify;
static ssl_get_verify_result_fn		__ssl_get_verify_result;
static ssl_get_peer_cert_fn		__ssl_get_peer_cert;
static ssl_get_bytes_avail_fn		__ssl_get_bytes_avail;

extern ACL_DLL_HANDLE __tls_dll;  // defined in mbedtls_conf.cpp

bool mbedtls_load_io(void)
{
#define LOAD(name, type, fn) do {					\
	(fn) = (type) acl_dlsym(__tls_dll, (name));			\
	if ((fn) == NULL) {						\
		logger_error("dlsym %s error %s", name, acl_dlerror());	\
		return false;						\
	}								\
} while (0)

	acl_assert(__tls_dll);

	LOAD(SSL_INIT_NAME, ssl_init_fn, __ssl_init);
	LOAD(SSL_FREE_NAME, ssl_free_fn, __ssl_free);
	LOAD(SSL_SET_HOSTNAME_NAME, ssl_set_hostname_fn, __ssl_set_hostname);
	LOAD(SSL_SET_BIO_NAME, ssl_set_bio_fn, __ssl_set_bio);
	LOAD(SSL_READ_NAME, ssl_read_fn, __ssl_read);
	LOAD(SSL_WRITE_NAME, ssl_write_fn, __ssl_write);
	LOAD(SSL_HANDSHAKE_NAME, ssl_handshake_fn, __ssl_handshake);
	LOAD(SSL_SET_SESSION_NAME, ssl_set_session_fn, __ssl_set_session);
	LOAD(SSL_SESSION_FREE_NAME, ssl_session_free_fn, __ssl_session_free);
	LOAD(SSL_SESSION_RESET_NAME, ssl_session_reset_fn, __ssl_session_reset);
	LOAD(SSL_CLOSE_NOTIFY_NAME, ssl_close_notify_fn, __ssl_close_notify);
	LOAD(SSL_GET_VERIFY_RESULT_NAME, ssl_get_verify_result_fn, __ssl_get_verify_result);
	LOAD(SSL_GET_PEER_CERT_NAME, ssl_get_peer_cert_fn, __ssl_get_peer_cert);
	LOAD(SSL_GET_BYTES_AVAIL_NAME, ssl_get_bytes_avail_fn, __ssl_get_bytes_avail);
	return true;
}

#elif defined(HAS_MBEDTLS)

# define __ssl_init			::mbedtls_ssl_init
# define __ssl_free			::mbedtls_ssl_free
# define __ssl_set_hostname		::mbedtls_ssl_set_hostname
# define __ssl_set_bio			::mbedtls_ssl_set_bio
# define __ssl_read			::mbedtls_ssl_read
# define __ssl_write			::mbedtls_ssl_write
# define __ssl_handshake		::mbedtls_ssl_handshake
# define __ssl_set_session		::mbedtls_ssl_set_session
# define __ssl_session_free		::mbedtls_ssl_session_free
# define __ssl_session_reset		::mbedtls_ssl_session_reset
# define __ssl_close_notify		::mbedtls_ssl_close_notify
# define __ssl_get_verify_result	::mbedtls_ssl_get_verify_result
# define __ssl_get_peer_cert		::mbedtls_ssl_get_peer_cert
# define __ssl_get_bytes_avail		::mbedtls_ssl_get_bytes_avail

#endif

namespace acl {

mbedtls_io::mbedtls_io(mbedtls_conf& conf, bool server_side,
	bool nblock /* = false */)
: sslbase_io(conf, server_side, nblock)
, conf_(conf)
, ssl_(NULL)
, ssn_(NULL)
{
#ifdef HAS_MBEDTLS
	conf.init_once();
#else
	(void) conf_;
	(void) ssl_;
	(void) ssn_;
#endif
}

mbedtls_io::~mbedtls_io(void)
{
#ifdef HAS_MBEDTLS
	if (ssl_) {
		__ssl_free((mbedtls_ssl_context*) ssl_);
		acl_myfree(ssl_);
	}
	if (ssn_) {
		__ssl_session_free((mbedtls_ssl_session*) ssn_);
		acl_myfree(ssn_);
	}
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

	char host[128];
	host[0] = 0;

	char* ptr = ACL_VSTREAM_PEER(s);
	if (ptr && *ptr) {
		safe_snprintf(host, sizeof(host), "%s", ptr);
	} else if (acl_getpeername(ACL_VSTREAM_SOCK(s), host, sizeof(host))) {
		logger_error("can't acl_getpeername error=%s", last_serror());
		return false;
	}

	ptr = strrchr(host, '|');
	if (ptr == NULL) {
		ptr = strrchr(host, ':');
	}
	if (ptr) {
		*ptr = 0;
	}

	stream_ = s;
	++(*refers_);

	ssl_ = acl_mycalloc(1, sizeof(mbedtls_ssl_context));

	// 初始化 SSL 对象
	__ssl_init((mbedtls_ssl_context*) ssl_);

	if (!sni_host_.empty()) {
		__ssl_set_hostname((mbedtls_ssl_context*) ssl_, sni_host_.c_str());
	} else if (host[0]) {
		__ssl_set_hostname((mbedtls_ssl_context*) ssl_, host);
	}

	// 配置全局参数（包含证书、私钥）
	conf_.setup_certs(ssl_);

	// Setup SSL IO callback
	__ssl_set_bio((mbedtls_ssl_context*) ssl_, this,
		sock_send, sock_read, NULL);

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

			logger_error("ssl_handshake failed: -0x%04x", -ret);
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
	int ret = __ssl_get_verify_result((mbedtls_ssl_context*) ssl_);
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

		if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
			//acl_set_error(ACL_ENOTCONN);
			return ACL_VSTREAM_EOF;
		}
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
	size_t total_bytes = 0;
	int bytes_written = 0;

	while (total_bytes < len) {
		bytes_written = __ssl_write((mbedtls_ssl_context*) ssl_,
			(unsigned char*) buf + total_bytes, len - total_bytes);
		if (bytes_written == MBEDTLS_ERR_SSL_WANT_READ ||
			bytes_written == MBEDTLS_ERR_SSL_WANT_WRITE) {
			if (nblock_) {
				return ACL_VSTREAM_EOF;
			}
			continue;
		} else if (bytes_written < 0) {
			return ACL_VSTREAM_EOF;
		}

		total_bytes += bytes_written;
	}

	return (int) total_bytes;

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
