#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_connack.hpp"
#endif

namespace acl {

enum {
	MQTT_STAT_HDR_VAR,
};

mqtt_connack::mqtt_connack(void)
: mqtt_message(MQTT_CONNACK)
, finished_(false)
, dlen_(0)
, session_(false)
, conn_flags_(0)
, connack_code_(MQTT_CONNACK_OK)
{
	status_ = MQTT_STAT_HDR_VAR;  // just for update()
}

mqtt_connack::mqtt_connack(const mqtt_header& header)
: mqtt_message(header)
, finished_(false)
, dlen_(0)
, session_(false)
, conn_flags_(0)
, connack_code_(MQTT_CONNACK_OK)
{
	status_ = MQTT_STAT_HDR_VAR;  // just for update()
}

mqtt_connack::~mqtt_connack(void) {}

mqtt_connack& mqtt_connack::set_session(bool on) {
	session_ = on;
	if (on) {
		conn_flags_ = 0x01;
	} else {
		conn_flags_ = 0x00;
	}
	return *this;
}

mqtt_connack& mqtt_connack::set_connack_code(unsigned char code) {
	connack_code_ = code;
	return *this;
}

bool mqtt_connack::to_string(string& out) {
	mqtt_header& header = this->get_header();
	header.set_remaing_length(2);

	if (!header.build_header(out)) {
		return false;
	}

	this->pack_add((unsigned char) conn_flags_, out);
	this->pack_add((unsigned char) connack_code_, out);
	return true;
}

static struct {
	int status;
	int (mqtt_connack::*handler)(const char*, int);
} handlers[] = {
	{ MQTT_STAT_HDR_VAR,	&mqtt_connack::update_header_var	},
};

int mqtt_connack::update(const char* data, int dlen) {
	if (data == NULL || dlen  <= 0) {
		logger_error("invalid input");
		return -1;
	}

	while (dlen > 0 && !finished_) {
		int ret = (this->*handlers[status_].handler)(data, dlen);
		if (ret < 0) {
			return -1;
		}
		data += dlen - ret;
		dlen  = ret;
	}
	return dlen;
}

#define	HDR_VAR_LEN	2

int mqtt_connack::update_header_var(const char* data, int dlen) {
	assert(data && dlen > 0);
	assert(sizeof(buff_) >= HDR_VAR_LEN);

	if (dlen_ >= HDR_VAR_LEN) {
		logger_error("invalid header var");
		return -1;
	}

	for (; dlen_ < HDR_VAR_LEN && dlen > 0;) {
		buff_[dlen_++] = *data++;
		dlen --;
	}

	if (dlen_ < HDR_VAR_LEN) {
		assert(dlen == 0);
		return dlen;
	}

	conn_flags_   = buff_[0];
	connack_code_ = buff_[1];

	finished_ = true;
	return dlen;
}

} // namespace acl
