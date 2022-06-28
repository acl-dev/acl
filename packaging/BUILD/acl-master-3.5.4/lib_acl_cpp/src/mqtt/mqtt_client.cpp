#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_message.hpp"
#include "acl_cpp/mqtt/mqtt_client.hpp"
#endif

namespace acl {

mqtt_client::mqtt_client(const char* addr, int conn_timeout, int rw_timeout)
: addr_(addr)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
{
	conn_ = conn_internal_ = NEW socket_stream;
}

mqtt_client::mqtt_client(socket_stream& conn)
: rw_timeout_(0)
, conn_(&conn)
, conn_internal_(NULL)
{
}

mqtt_client::~mqtt_client(void) { delete conn_internal_; }

bool mqtt_client::open(void) {
	if (conn_->opened()) {
		return true;
	}

	if (!conn_->open(addr_, conn_timeout_, rw_timeout_)) {
		logger_error("connect redis %s error: %s",
			addr_.c_str(), last_serror());
		return false;
	}

	return true;
}

bool mqtt_client::send(mqtt_message& message) {
	if (!open()) {
		logger_error("connect server error: %s", last_serror());
		return false;
	}

	string buff;
	if (!message.to_string(buff)) {
		logger_error("build mqtt message error");
		return false;
	}
	if (buff.empty()) {
		logger_error("mqtt message empty");
		return false;
	}

	if (!open()) {
		logger_error("open error");
		return false;
	}

	if (conn_->write(buff) == -1) {
		conn_->close();
		logger_error("send message error=%s", last_serror());
		return false;
	}

	return true;
}

mqtt_message* mqtt_client::get_message(void) {
	mqtt_header header(MQTT_RESERVED_MIN);

	if (!read_header(header)) {
		conn_->close();
		logger_error("get header error");
		return NULL;
	}

	mqtt_message* message = mqtt_message::create_message(header);
	if (message == NULL) {
		logger_error("create_message error");
		return NULL;
	}

	if (!read_message(header, *message)) {
		delete message;
		return NULL;
	}
	return message;
}

bool mqtt_client::read_header(mqtt_header& header) {
	char ch;
	if (!conn_->read(ch)) {
		logger_error("read header type error: %s", last_serror());
		return false;
	}

	// update the first char for mqtt_type_t
	if (header.update(&ch, 1) != 0) {
		logger_error("invalid header type=%d", (int) ch);
		return false;
	}

	for (int i = 0; i < 4; i++) {
		if (!conn_->read(ch)) {
			logger_error("read one char error: %s, i=%d",
				last_serror(), i);
			return false;
		}
		if (header.update(&ch, 1) != 0) {
			logger_error("header_update error, ch=%d", (int) ch);
			return false;
		}
		if (header.finished()) {
			break;
		}
	}

	if (!header.finished()) {
		logger_error("get mqtt header error");
		return false;
	}

	return true;
}

bool mqtt_client::read_message(const mqtt_header& header, mqtt_message& body) {
	unsigned len = header.get_remaining_length();
	if (len == 0) {
		return true;
	}

	char buf[8192];
	while (len > 0) {
		size_t size = sizeof(buf) > len ? len : sizeof(buf);
		int n = conn_->read(buf, size);
		if (n == -1) {
			logger_error("read body error: %s", last_serror());
			return false;
		}

		len -= n;

		n = body.update(buf, (int) size);
		if (n == -1) {
			logger_error("update body error");
			return false;
		} else if (n != 0) {
			logger_error("invalid body data");
			return false;
		}
	}

	if (!body.finished()) {
		logger_error("body not finished!");
		return false;
	}
	return true;
}

} // namespace acl
