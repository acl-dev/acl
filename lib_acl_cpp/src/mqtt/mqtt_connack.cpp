#include "acl_stdafx.hpp"
#include "acl_cpp/mqtt/mqtt_connack.hpp"

namespace acl {

mqtt_connack::mqtt_connack(void)
: mqtt_message(MQTT_CONNACK)
, finished_(false)
, hlen_(0)
, session_(false)
, conn_flags_(0)
{
	status_ = MQTT_STAT_HDR_VAR;  // just for update()
}

mqtt_connack::~mqtt_connack(void) {}

void mqtt_connack::set_session(bool on) {
	session_ = on;
}

static struct {
	int status;
	int (mqtt_connack::*handler)(const char*, unsigned);
} connack_handlers[] = {
	{ MQTT_STAT_STR_LEN,	&mqtt_message::unpack_string_len	},
	{ MQTT_STAT_STR_VAL,	&mqtt_message::unpack_string_val	},

	{ MQTT_STAT_HDR_VAR,	&mqtt_connack::unpack_header_var	},
};

int mqtt_connack::update(const char* data, unsigned dlen) {
	if (data == NULL || dlen  == 0) {
		logger_error("invalid input");
		return -1;
	}

	while (dlen > 0 && !finished_) {
		int ret = (this->*connack_handlers[status_].handler)(data, dlen);
		if (ret < 0) {
			return -1;
		}
		dlen = (unsigned) ret;
		data += ret;
	}
	return dlen;
}

#define	HDR_VAR_LEN	2

int mqtt_connack::unpack_header_var(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		return 0;
	}
	assert(sizeof(hbuf_) >= HDR_VAR_LEN);

	if (hlen_ >= HDR_VAR_LEN) {
		logger_error("invalid header var");
		return -1;
	}

	for (; hlen_ < HDR_VAR_LEN && dlen > 0;) {
		hbuf_[hlen_++] = *data++;
		dlen --;
	}

	if (hlen_ < HDR_VAR_LEN) {
		assert(dlen == 0);
		return dlen;
	}

	finished_ = true;
	return dlen;

}

} // namespace acl
