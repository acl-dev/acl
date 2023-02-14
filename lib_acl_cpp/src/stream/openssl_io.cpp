#include "acl_stdafx.hpp"

#ifndef ACL_PREPARE_COMPILE
# include "acl_cpp/stream/openssl_conf.hpp"
# include "acl_cpp/stream/openssl_io.hpp"
#endif

//#define HAS_OPENSSL
//#define HAS_OPENSSL_DLL

#ifdef HAS_OPENSSL_DLL
# ifndef HAS_OPENSSL
#  define HAS_OPENSSL
# endif
#endif

#ifdef HAS_OPENSSL
#include "openssl/ssl.h"
#include "openssl/err.h"
#endif

#if defined(HAS_OPENSSL_DLL)

# define SSL_NEW			"SSL_new"
typedef SSL* (*ssl_new_fn)(SSL_CTX*);
static ssl_new_fn __ssl_new;

# define SSL_FREE			"SSL_free"
typedef void (*ssl_free_fn)(SSL*);
static ssl_free_fn __ssl_free;

# define SSL_SET_FD			"SSL_set_fd"
typedef int (*ssl_set_fd_fn)(SSL*, int);
static ssl_set_fd_fn __ssl_set_fd;

# define SSL_CTRL			"SSL_ctrl"
typedef long (*ssl_ctrl_fn)(SSL *ssl, int, long, void*);
static ssl_ctrl_fn __ssl_ctrl;

# define SSL_SET_ACCEPT_STATE		"SSL_set_accept_state"
typedef void (*ssl_set_accept_state_fn)(SSL*);
static ssl_set_accept_state_fn __ssl_set_accept_state;

# define SSL_SET_CONNECT_STATE		"SSL_set_connect_state"
typedef void (*ssl_set_connect_state_fn)(SSL*);
static ssl_set_connect_state_fn __ssl_set_connect_state;

# define SSL_DO_HANDSHAKE		"SSL_do_handshake"
typedef int (*ssl_do_handshake_fn)(SSL*);
static ssl_do_handshake_fn __ssl_do_handshake;

# define SSL_GET_ERROR			"SSL_get_error"
typedef int (*ssl_get_error_fn)(const SSL*, int);
static ssl_get_error_fn __ssl_get_error;

# define ERR_PEEK_ERROR			"ERR_peek_error"
typedef unsigned long (*err_peek_error_fn)(void);
static err_peek_error_fn __err_peek_error;

# define SSL_IN_INIT			"SSL_in_init"
typedef int (*ssl_in_init_fn)(const SSL*);
static ssl_in_init_fn __ssl_in_init;

# define SSL_GET_SHUTDOWN		"SSL_get_shutdown"
typedef int (*ssl_get_shutdown_fn)(const SSL*);
static ssl_get_shutdown_fn __ssl_get_shutdown;

# define SSL_SET_QUIET_SHUTDOWN		"SSL_set_quiet_shutdown"
typedef void (*ssl_set_quiet_shutdown_fn)(SSL*, int);
static ssl_set_quiet_shutdown_fn __ssl_set_quiet_shutdown;

# define SSL_SET_SHUTDOWN		"SSL_set_shutdown"
typedef void (*ssl_set_shutdown_fn)(SSL*, int);
static ssl_set_shutdown_fn __ssl_set_shutdown;

# define SSL_SHUTDOWN			"SSL_shutdown"
typedef int (*ssl_shutdown_fn)(SSL*);
static ssl_shutdown_fn __ssl_shutdown;

# define SSL_READ			"SSL_read"
typedef int (*ssl_read_fn)(SSL*, void*, int);
static ssl_read_fn __ssl_read;

# define SSL_WRITE			"SSL_write"
typedef int (*ssl_write_fn)(SSL*, const void*, int);
static ssl_write_fn __ssl_write;

extern ACL_DLL_HANDLE __openssl_ssl_dll;  // defined in openssl_conf.cpp
extern ACL_DLL_HANDLE __openssl_crypto_dll;  // defined in openssl_conf.cpp

bool openssl_load_io(void)
{
#define LOAD(name, type, fn) do {                               \
    (fn) = (type) acl_dlsym(__openssl_ssl_dll, (name));         \
    if ((fn) == NULL) {                                         \
        logger_error("dlsym %s error %s", name, acl_dlerror()); \
        return false;                                           \
    }                                                           \
} while (0)

#define LOAD_CRYPTO(name, type, fn) do {                        \
    (fn) = (type) acl_dlsym(__openssl_crypto_dll, (name));      \
    if ((fn) == NULL) {                                         \
        logger_error("dlsym %s error %s", name, acl_dlerror()); \
        return false;                                           \
    }                                                           \
} while (0)

	acl_assert(__openssl_ssl_dll);

	LOAD(SSL_NEW, ssl_new_fn, __ssl_new);
	LOAD(SSL_FREE, ssl_free_fn, __ssl_free);
	LOAD(SSL_SET_FD, ssl_set_fd_fn, __ssl_set_fd);
	LOAD(SSL_CTRL, ssl_ctrl_fn, __ssl_ctrl);
	LOAD(SSL_SET_ACCEPT_STATE, ssl_set_accept_state_fn, __ssl_set_accept_state);
	LOAD(SSL_SET_CONNECT_STATE, ssl_set_connect_state_fn, __ssl_set_connect_state);
	LOAD(SSL_DO_HANDSHAKE, ssl_do_handshake_fn, __ssl_do_handshake);
	LOAD(SSL_GET_ERROR, ssl_get_error_fn, __ssl_get_error);
#if defined(_WIN32) || defined(_WIN64)
	LOAD_CRYPTO(ERR_PEEK_ERROR, err_peek_error_fn, __err_peek_error);
#else
	LOAD(ERR_PEEK_ERROR, err_peek_error_fn, __err_peek_error);
#endif
	LOAD(SSL_IN_INIT, ssl_in_init_fn, __ssl_in_init);
	LOAD(SSL_GET_SHUTDOWN, ssl_get_shutdown_fn, __ssl_get_shutdown);
	LOAD(SSL_SET_QUIET_SHUTDOWN, ssl_set_quiet_shutdown_fn, __ssl_set_quiet_shutdown);
	LOAD(SSL_SET_SHUTDOWN, ssl_set_shutdown_fn, __ssl_set_shutdown);
	LOAD(SSL_SHUTDOWN, ssl_shutdown_fn, __ssl_shutdown);
	LOAD(SSL_READ, ssl_read_fn, __ssl_read);
	LOAD(SSL_WRITE, ssl_write_fn, __ssl_write);

	return true;
}

#else  // !HAS_OPENSSL_DLL && HAS_OPENSSL

# define __ssl_new			SSL_new
# define __ssl_free			SSL_free
# define __ssl_set_fd			SSL_set_fd
# define __ssl_ctrl			SSL_ctrl
# define __ssl_set_accept_state		SSL_set_accept_state
# define __ssl_set_connect_state	SSL_set_connect_state
# define __ssl_do_handshake		SSL_do_handshake
# define __ssl_get_error		SSL_get_error
# define __err_peek_error		ERR_peek_error
# define __ssl_in_init			SSL_in_init
# define __ssl_get_shutdown		SSL_get_shutdown
# define __ssl_set_quiet_shutdown	SSL_set_quiet_shutdown
# define __ssl_set_shutdown		SSL_set_shutdown
# define __ssl_shutdown			SSL_shutdown
# define __ssl_read			SSL_read
# define __ssl_write			SSL_write

#endif // !HAS_OPENSSL_DLL

namespace acl {

openssl_io::openssl_io(openssl_conf& conf, bool server_side, bool nblock)
: sslbase_io(conf, server_side, nblock)
, conf_(conf)
, ssl_(NULL)
{
#ifndef HAS_OPENSSL
	(void) conf_;
	(void) ssl_;
#endif
}

openssl_io::~openssl_io(void)
{
#ifdef HAS_OPENSSL
	if (ssl_) {
		__ssl_free(ssl_);
	}
#endif
}

void openssl_io::destroy(void)
{
	if (--(*refers_) <= 0) {
		delete this;
	}
}

bool openssl_io::open(ACL_VSTREAM* s)
{
	if (s == NULL) {
		logger_error("s null");
		return false;
	}

	this->stream_ = s;
	++(*refers_);

#ifdef HAS_OPENSSL
	SSL_CTX* ctx = conf_.get_ssl_ctx();
	if (ctx == NULL) {
		logger_error("The default SSL_CTX in conf_ is null");
		return false;
	}

	SSL* ssl = __ssl_new(ctx);

	if (!__ssl_set_fd(ssl, ACL_VSTREAM_SOCK(s))) {
		logger_error("SSL_set_fd error");
		__ssl_free(ssl);
		return false;
	}

	if (conf_.is_server_side()) {
		__ssl_set_accept_state(ssl);
	} else {
		if (!sni_host_.empty()) {
			__ssl_ctrl(ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME,
				TLSEXT_NAMETYPE_host_name,
				(void*) sni_host_.c_str());
		}
		__ssl_set_connect_state(ssl);
	}

	ssl_ = ssl;

	if (nblock_) {
		return true;
	}

	return handshake();
#else
	logger_error("define HAS_OPENSSL first!");
	return false;
#endif
}

bool openssl_io::handshake(void)
{
	if (handshake_ok_) {
		return true;
	}

#ifdef HAS_OPENSSL
	while (true) {
		int ret = __ssl_do_handshake(ssl_);
		if (ret == 1) {
			handshake_ok_ = true;
			return true;
		}

		int err = __ssl_get_error(ssl_, ret);
		switch (err) {
		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
			if (nblock_) {
				return true;
			}
			break;
		case SSL_ERROR_ZERO_RETURN:
		default:
			return false;
		}
	}
#else
	logger_error("define HAS_OPENSSL first!");
	return false;
#endif
}

bool openssl_io::on_close(bool alive)
{
	if (!alive) {
		return false;
	}

#ifdef HAS_OPENSSL
	if (__ssl_in_init(ssl_)) {
		// OpenSSL 1.0.2f complains if SSL_shutdown() is called during
		// an SSL handshake, while previous versions always return 0.
		// Avoid calling SSL_shutdown() if handshake wasn't completed.
		// -- nginx
		return true;
	}

	int mode = __ssl_get_shutdown(ssl_);
	mode |= SSL_RECEIVED_SHUTDOWN;
	mode |= SSL_SENT_SHUTDOWN;
	__ssl_set_quiet_shutdown(ssl_, 1);
	__ssl_set_shutdown(ssl_, mode);

	int ret = __ssl_shutdown(ssl_);
	if (ret == 1 || __err_peek_error() == 0) {
		return true;
	}

	int err = __ssl_get_error(ssl_, ret);
	switch (err) {
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_WRITE:
		break;
	default:
		break;
	}
	return true;
#else
	logger_error("define HAS_OPENSSL first!");
	return false;
#endif
}

int openssl_io::read(void* buf, size_t len)
{
#ifdef HAS_OPENSSL
	size_t nbytes = 0;
	char*  ptr = (char*) buf;
	int timeout = nblock_ ? 0 : this->stream_->rw_timeout;
	ACL_SOCKET fd = ACL_VSTREAM_SOCK(this->stream_);

	while (len > 0) {
		time_t begin = time(NULL);
		if (acl_read_wait(fd, timeout) < 0) {
			time_t end = time(NULL);
			logger_error("acl_read_wait error=%s, fd=%d, cost=%ld",
				last_serror(), (int) fd, (long) (end - begin));
			return -1;
		}

		int ret = __ssl_read(ssl_, ptr, len);
		if (ret > 0) {
			nbytes += ret;
			ptr += ret;
			len -= ret;

			break;
		}

		if (nbytes > 0) {
			break;
		}

		this->stream_->read_ready = 0;

		int err = __ssl_get_error(ssl_, ret);

		switch (err) {
		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
			if (nblock_) {
				return ACL_VSTREAM_EOF;
			}
			break;
		case SSL_ERROR_ZERO_RETURN:
		case SSL_ERROR_SYSCALL:
		default:
			/*
			if (ERR_peek_error() == 0) {
			}
			*/
			return ACL_VSTREAM_EOF;
		}
	}

	if (len == 0) {
		this->stream_->read_ready = 1;
	}

	return nbytes > 0 ? (int) nbytes : ACL_VSTREAM_EOF;
#else
	(void) buf;
	(void) len;
	logger_error("define HAS_OPENSSL first!");
	return false;
#endif
}

int openssl_io::send(const void* buf, size_t len)
{
#ifdef HAS_OPENSSL
	size_t nbytes = 0;
	char*  ptr = (char*) buf;
	int timeout = nblock_ ? 0 : this->stream_->rw_timeout;
	ACL_SOCKET fd = ACL_VSTREAM_SOCK(this->stream_);

	while (len > 0) {
		time_t begin = time(NULL);
		if (acl_write_wait(fd, timeout) < 0) {
			time_t end = time(NULL);
			logger_error("acl_write_wait error=%s, fd=%d, cost=%ld",
				last_serror(), (int) fd, (long) (end - begin));
			return -1;
		}

		int ret = __ssl_write(ssl_, ptr, len);
		if (ret > 0) {
			nbytes += ret;
			ptr += ret;
			len -= ret;

			break;
		}

		int err = __ssl_get_error(ssl_, ret);
		switch (err) {
		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
			break;
		case SSL_ERROR_SYSCALL:
		default:
			break;
		}
		break;
	}

	return nbytes > 0 ? (int) nbytes : ACL_VSTREAM_EOF;
#else
	(void) buf;
	(void) len;
	logger_error("define HAS_OPENSSL first!");
	return false;
#endif
}

} // namespace acl
