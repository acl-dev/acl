#include "acl_stdafx.hpp"

//#ifndef ACL_PREPARE_COMPILE
# include "acl_cpp/stream/openssl_conf.hpp"
# include "acl_cpp/stream/openssl_io.hpp"
//#endif

#ifdef HAS_OPENSSL_DLL
# ifndef HAS_OPENSSL
#  define HAS_OPENSSL
# endif
#endif

#ifdef HAS_OPENSSL
#include <openssl/ssl.h>
#endif
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace acl {

openssl_io::openssl_io(openssl_conf& conf, bool server_side, bool nblock)
: sslbase_io(conf, server_side, nblock)
, conf_(conf)
, ssl_(NULL)
{
}

openssl_io::~openssl_io(void)
{
	if (ssl_) {
		SSL_free((SSL*) ssl_);
	}
}

void openssl_io::destroy(void)
{
	if (--(refers_) == 0) {
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

	SSL* ssl = SSL_new((SSL_CTX*) conf_.get_ssl_ctx());

	if (!SSL_set_fd(ssl, ACL_VSTREAM_SOCK(s))) {
		logger_error("SSL_set_fd error");
		SSL_free(ssl);
		return false;
	}

	if (conf_.is_server_side()) {
		SSL_set_accept_state(ssl);
	} else {
		SSL_set_connect_state(ssl);
	}

	ssl_ = ssl;

	if (nblock_) {
		return true;
	}

	return handshake();
}

bool openssl_io::handshake(void)
{
	if (handshake_ok_) {
		return true;
	}

	int ret = SSL_do_handshake((SSL*) ssl_);
	if (ret == 1) {
		handshake_ok_ = true;
		return true;
	}

	int err = SSL_get_error((SSL*) ssl_, ret);
	switch (err) {
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_WRITE:
		return true;
	case SSL_ERROR_ZERO_RETURN:
	default:
		return false;
	}
}

bool openssl_io::on_close(bool alive)
{
	if (!alive) {
		return false;
	}

	if (SSL_in_init((SSL*) ssl_)) {
		// OpenSSL 1.0.2f complains if SSL_shutdown() is called during
		// an SSL handshake, while previous versions always return 0.
		// Avoid calling SSL_shutdown() if handshake wasn't completed.
		// -- nginx
		return true;
	}

	int mode = SSL_get_shutdown((SSL*) ssl_);
	mode |= SSL_RECEIVED_SHUTDOWN;
	mode |= SSL_SENT_SHUTDOWN;
	SSL_set_quiet_shutdown((SSL*) ssl_, 1);
	SSL_set_shutdown((SSL*) ssl_, mode);

	int ret = SSL_shutdown((SSL*) ssl_);
	if (ret == 1 || ERR_peek_error() == 0) {
		return true;
	}

	int err = SSL_get_error((SSL*) ssl_, ret);
	switch (err) {
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_WRITE:
		break;
	default:
		break;
	}
	return true;
}

int openssl_io::read(void* buf, size_t len)
{
	size_t nbytes = 0;
	char*  ptr = (char*) buf;

	while (len > 0) {
		int ret = SSL_read((SSL*) ssl_, ptr, len);
		if (ret > 0) {
			nbytes += ret;
			ptr += ret;
			len -= ret;

			if (nblock_) {
				break;
			}
			continue;
		}

		int err = SSL_get_error((SSL*) ssl_, ret);
		switch (err) {
		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
			this->stream_->read_ready = 0;
			break;
		case SSL_ERROR_ZERO_RETURN:
		case SSL_ERROR_SYSCALL:
		default:
			/*
			if (ERR_peek_error() == 0) {
			}
			*/
			break;
		}

		break;
	}

	if (len == 0) {
		this->stream_->read_ready = 1;
	}

	return nbytes > 0 ? (int) nbytes : ACL_VSTREAM_EOF;
}

int openssl_io::send(const void* buf, size_t len)
{
	size_t nbytes = 0;
	char*  ptr = (char*) buf;

	while (len > 0) {
		int ret = SSL_write((SSL*) ssl_, ptr, len);
		if (ret > 0) {
			nbytes += ret;
			ptr += ret;
			len -= ret;

			if (nblock_) {
				break;
			}
			continue;
		}

		int err = SSL_get_error((SSL*) ssl_, ret);
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
}

} // namespace acl
