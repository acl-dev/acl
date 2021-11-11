#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/sslbase_conf.hpp"
#include "acl_cpp/stream/sslbase_io.hpp"
#include "acl_cpp/mqtt/mqtt_message.hpp"
#include "acl_cpp/mqtt/mqtt_aclient.hpp"
#endif

namespace acl {

mqtt_aclient::mqtt_aclient(aio_handle& handle, sslbase_conf* ssl_conf)
: handle_(handle)
, ssl_conf_(ssl_conf)
, conn_(NULL)
, body_(NULL)
{
	header_ = NEW mqtt_header(MQTT_RESERVED_MIN);
}

mqtt_aclient::~mqtt_aclient(void) {
	delete header_;
	delete body_;
}

void mqtt_aclient::set_host(const char* host) {
	if (host && *host) {
		host_ = host;
	}
}

bool mqtt_aclient::open(const char* addr, int conn_timeout, int rw_timeout) {
	ACL_AIO* aio = handle_.get_handle();
	if (acl_aio_connect_addr(aio, addr, conn_timeout,
		  connect_callback, this) == -1) {
		logger_error("connect %s error %s", addr, last_serror());
		return false;
	}
	rw_timeout_ = rw_timeout;
	return true;
}

bool mqtt_aclient::open(aio_socket_stream* conn) {
	conn_ = conn;
	conn_->add_close_callback(this);
	conn_->add_timeout_callback(this);
	if (!ssl_conf_) {
		return open_done();
	}

	sslbase_io* ssl_io = ssl_conf_->create(true);

	if (conn_->setup_hook(ssl_io) == ssl_io || !ssl_io->handshake()) {
		logger_error("open ssl failed");
		conn_->remove_hook();
		ssl_io->destroy();
		return false;
	}

	// begin SSL handshake, read_wakeup will be called if some data arrived.
	conn_->add_read_callback(this);
	conn_->readable_await(rw_timeout_);
	return true;

}

void mqtt_aclient::close(void) {
	if (conn_) {
		conn_->close();
	}
}

bool mqtt_aclient::get_ns_addr(string& out) const {
	char buf[256];
	const struct sockaddr* sa = (const struct sockaddr*) &ns_addr_;
	size_t ret = acl_inet_ntop(sa, buf, sizeof(buf));
	if (ret == 0) {
		return false;
	}
	out = buf;
	return true;
}

bool mqtt_aclient::get_server_addr(string& out) const {
	char buf[256];
	const struct sockaddr* sa = (const struct sockaddr*) &serv_addr_;
	size_t ret = acl_inet_ntop(sa, buf, sizeof(buf));
	if (ret == 0) {
		return false;
	}
	out = buf;
	return true;
}

bool mqtt_aclient::handle_connect(const ACL_ASTREAM_CTX *ctx)
{
	const ACL_SOCKADDR *ns_addr = acl_astream_get_ns_addr(ctx);
	if (ns_addr) {
		memcpy(&ns_addr_, &ns_addr->ss, sizeof(ns_addr_));
	}
	const ACL_SOCKADDR* serv_addr = acl_astream_get_serv_addr(ctx);
	if (serv_addr) {
		memcpy(&serv_addr_, &serv_addr->ss, sizeof(serv_addr_));
	}

	ACL_ASTREAM* astream = acl_astream_get_conn(ctx);
	if (astream == NULL) {
		int status = acl_astream_get_status(ctx);
		switch (status) {
		case ACL_ASTREAM_STATUS_NS_ERROR:
			this->on_ns_failed();
			this->destroy();
			break;
		case ACL_ASTREAM_STATUS_CONNECT_TIMEOUT:
			this->on_connect_timeout();
			this->destroy();
			break;
		default:
			this->on_connect_failed();
			this->destroy();
			break;
		}
		return false;
	}

	// create one c++ aio with the connection
	conn_ = NEW aio_socket_stream(&handle_, astream, true);

	// add closing callback when the connection disconnected
	conn_->add_close_callback(this);

	// add reading timeout callback when reading timeout
	conn_->add_timeout_callback(this);

	if (!ssl_conf_) {
		return open_done();
	}

	// create one SSL IO for SSL communication mode, and begin to SSL
	// handshake async.
	sslbase_io* ssl_io = ssl_conf_->create(true);

	if (!host_.empty()) {
		ssl_io->set_sni_host(host_);
	}

	if (conn_->setup_hook(ssl_io) == ssl_io || !ssl_io->handshake()) {
		logger_error("open ssl failed");
		conn_->remove_hook();
		ssl_io->destroy();
		this->on_connect_failed();
		return false;
	}

	// begin SSL handshake, read_wakeup will be called if some data arrived.
	conn_->add_read_callback(this);
	conn_->readable_await(rw_timeout_);
	return true;
}

int mqtt_aclient::connect_callback(const ACL_ASTREAM_CTX* ctx) {
	assert(ctx);
	mqtt_aclient* me = (mqtt_aclient*) acl_astream_get_ctx(ctx);
	assert(me);
	return me->handle_connect(ctx) ? 0 : -1;
}

bool mqtt_aclient::open_done(void) {
	if (!this->on_open()) {
		return false;
	}

	// begin to wait for mqtt message from peer
	return message_await();
}

bool mqtt_aclient::timeout_callback(void) {
	return this->on_read_timeout();
}

void mqtt_aclient::close_callback(void) {
	this->on_disconnect();
	this->destroy();
}

bool mqtt_aclient::read_wakeup(void) {
	return handle_ssl_handshake();
}

bool mqtt_aclient::handle_ssl_handshake(void) {
	sslbase_io* ssl_io = (sslbase_io*) conn_->get_hook();
	if (ssl_io == NULL) {
		logger_error("no ssl_io hooked!");
		return false;
	}

	if (!ssl_io->handshake()) {
		logger_error("ssl handshake error!");
		return false;
	}

	// if ssl handshake successful, notify the subclass object
	if (ssl_io->handshake_ok()) {
		conn_->del_read_callback(this);
		conn_->disable_read();
		return open_done();
	}

	// else continue to wait for the completion of ssl handshake
	return true;
}

bool mqtt_aclient::send(mqtt_message& message) {
	mqtt_type_t type = message.get_header().get_type();
	if (conn_ == NULL) {
		logger_error("connection not opened yet, type=%s",
			mqtt_type_desc(type));
		return false;
	}

	string buff;
	if (!message.to_string(buff)) {
		logger_error("build mqtt message error, type=%s",
			mqtt_type_desc(type));
		return false;
	}
	if (buff.empty()) {
		logger_error("message empty, type=%s",
			mqtt_type_desc(type));
		return false;
	}

	conn_->write(buff.c_str(), (int) buff.size());
	return true;
}

bool mqtt_aclient::message_await(void) {
	if (conn_) {
		conn_->keep_read(true);
		conn_->add_read_callback(this);
		conn_->read();
		return true;
	} else {
		logger_error("not connected yet");
		return false;
	}
}

bool mqtt_aclient::read_callback(char* data, int len) {
	if (data == NULL || len <= 0) {
		logger_error("invalid data=%p, len=%d", data, len);
		return false;
	}

	while (true) {
		int left = handle_data(data, len);
		if (left < 0) {
			header_->reset();
			return false;
		} else if (left > 0) {
			data += len - left;
			len   = left;
		} else {
			return true;
		}
	}
}

int mqtt_aclient::handle_data(char* data, int len) {
	int left;

	if (!header_->finished()) {
		left = header_->update(data, len);
	} else {
		left = len;
	}

	if (left < 0) {
		logger_error("header update failed");
		return -1;
	}

	if (!header_->finished()) {
		assert(left == 0);
		return 0;
	}

	data += len - left;
	len   = left;

	if (body_ == NULL) {
		body_ = mqtt_message::create_message(*header_);
		if (body_ == NULL) {
			logger_error("create mqtt_message failed");
			return -1;
		}
	}

	if (len > 0) {
		left = body_->update(data, len);
		if (left < 0) {
			logger_error("message update failed");
			return -1;
		}
	}

	if (body_->finished()) {
		bool ret = this->on_body(*body_);
		header_->reset();
		delete body_;
		body_ = NULL;

		if (!ret) {
			logger_error("subclass return false");
			return -1;
		}
	}

	return left;
}
} // namespace acl
