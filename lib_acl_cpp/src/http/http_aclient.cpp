#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"
#include "acl_cpp/stream/polarssl_conf.hpp"
#include "acl_cpp/stream/polarssl_io.hpp"
#include "acl_cpp/http/http_header.hpp"
#include "acl_cpp/http/http_aclient.hpp"
#endif

namespace acl
{

http_aclient::http_aclient(aio_handle& handle, polarssl_conf* ssl_conf /* NULL */)
: handle_(handle)
, ssl_conf_(ssl_conf)
, conn_(NULL)
, hdr_res_(NULL)
, http_res_(NULL)
{
	header_ = NEW http_header;
}

http_aclient::~http_aclient(void)
{
	if (http_res_) {
		http_res_free(http_res_);
	} else if (hdr_res_) {
		http_hdr_res_free(hdr_res_);
	}
	delete header_;
}

http_header& http_aclient::request_header(void)
{
	return *header_;
}

bool http_aclient::open(const char* addr, int conn_timeout)
{
	ACL_AIO* aio = handle_.get_handle();
	if (acl_aio_connect_addr(aio, addr, conn_timeout,
		connect_callback, this) == -1) {

		logger_error("connect %s error %s", addr, last_serror());
		return false;
	}
	return true;
}

int http_aclient::connect_callback(ACL_ASTREAM *stream, void *ctx)
{
	http_aclient* me = (http_aclient*) ctx;

	if (stream == NULL) {
		me->on_connect_failed();
		return -1;
	}

	me->conn_ = new aio_socket_stream(&me->handle_, stream, true);
	me->conn_->add_close_callback(me);

	if (!me->ssl_conf_) {
		return me->on_connect() ? 0 : -1;
	}

	polarssl_io* ssl_io = new polarssl_io(*me->ssl_conf_, false, true);
	if (me->conn_->setup_hook(ssl_io) == ssl_io || !ssl_io->handshake()) {
		logger_error("open ssl failed");
		me->conn_->remove_hook();
		ssl_io->destroy();
		me->on_connect_failed();
		return -1;
	}

	me->conn_->add_read_callback(me);
	me->conn_->read_wait(0);
	return 0;
}

bool http_aclient::open_callback(void)
{
	return true;
}

bool http_aclient::timeout_callback(void)
{
	return false;
}

void http_aclient::close_callback(void)
{
	this->on_disconnect();
}

bool http_aclient::read_wakeup(void)
{
	polarssl_io* ssl_io = (polarssl_io*) conn_->get_hook();
	if (ssl_io == NULL) {
		logger_error("no ssl_io hooked!");
		return false;
	}
	if (!ssl_io->handshake()) {
		logger_error("ssl handshake error!");
		return false;
	}
	if (ssl_io->handshake_ok()) {
		conn_->del_read_callback(this);
		conn_->disable_read();
		return this->on_connect();
	}

	return true;
}

bool http_aclient::read_callback(char* data, int len)
{
	(void) data;
	(void) len;
	return true;
}

int http_aclient::http_res_hdr_cllback(int status, void* ctx)
{
	http_aclient* me = (http_aclient*) ctx;
	acl_assert(status == HTTP_CHAT_OK);

	http_hdr_res_parse(me->hdr_res_);

	http_header header(*me->hdr_res_);
	if (!me->on_http_res_hdr(header)) {
		return -1;
	}

	me->http_res_ = http_res_new(me->hdr_res_);
	http_res_body_get_async(me->http_res_, me->conn_->get_astream(),
		http_res_callback, me, 0);
	return 0;
}

int http_aclient::http_res_callback(int status, char* data, int dlen, void* ctx)
{
	http_aclient* me = (http_aclient*) ctx;
	switch (status) {
	case HTTP_CHAT_CHUNK_HDR:
	case HTTP_CHAT_CHUNK_DATA_ENDL:
	case HTTP_CHAT_CHUNK_TRAILER:
		return 0;
	case HTTP_CHAT_OK:
		if (data == NULL || dlen <= 0) {
			return me->on_http_res_finish();
		}
		if (!me->on_http_res_body(data, (size_t) dlen)) {
			return -1;
		}
		return me->on_http_res_finish() ? 0 : -1;
	case HTTP_CHAT_ERR_IO:
	case HTTP_CHAT_ERR_PROTO:
		(void) me->on_http_res_finish();
		return -1;
	case HTTP_CHAT_DATA:
		return me->on_http_res_body(data, (size_t) dlen) ? 0 : -1;
	default:
		return 0;
	}
}

void http_aclient::send_request(const void* body, size_t len)
{
	http_method_t method = header_->get_method();
	if (body && len > 0 && method != HTTP_METHOD_POST
		&& method != HTTP_METHOD_PUT) {

		header_->set_content_length(len);
		header_->set_method(HTTP_METHOD_POST);
	}

	string buf;
	header_->build_request(buf);
	conn_->write(buf.c_str(), (int) buf.size());
	if (body && len > 0) {
		conn_->write(body, (int) len);
	}

	hdr_res_ = http_hdr_res_new();
	http_hdr_res_get_async(hdr_res_, conn_->get_astream(),
		http_res_hdr_cllback, this, 0);
}

} // namespace acl
