#include "acl_stdafx.hpp"
#include "acl_cpp/mqtt/mqtt_puback.hpp"

namespace acl {

mqtt_puback::mqtt_puback(mqtt_type_t type /* MQTT_PUBACK */)
: mqtt_message(type)
, finished_(false)
, hlen_(0)
, pkt_id_(0)
{
}

mqtt_puback::~mqtt_puback(void) {}

void mqtt_puback::set_pkt_id(unsigned short id) {
	pkt_id_ = id;
}

bool mqtt_puback::to_string(string& out) {
	bool old_mode = out.get_bin();

	this->set_payload_length(2);

	if (!this->pack_header(out)) {
		out.set_bin(old_mode);
		return false;
	}

	this->pack_add((unsigned short) pkt_id_, out);

	out.set_bin(old_mode);
	return true;
}

static struct {
	int status;
	int (mqtt_puback::*handler)(const char*, unsigned);
} handlers[] = {
	{ MQTT_STAT_STR_LEN,	&mqtt_message::unpack_string_len	},
	{ MQTT_STAT_STR_VAL,	&mqtt_message::unpack_string_val	},

	{ MQTT_STAT_HDR_VAR,	&mqtt_puback::unpack_header_var		},
};

int mqtt_puback::update(const char* data, unsigned dlen) {
	if (data == NULL || dlen  == 0) {
		logger_error("invalid input");
		return -1;
	}

	while (dlen > 0 && !finished_) {
		int ret = (this->*handlers[status_].handler)(data, dlen);
		if (ret < 0) {
			return -1;
		}
		dlen = (unsigned) ret;
		data += ret;
	}
	return dlen;
}

#define	HDR_VAR_LEN	2

int mqtt_puback::unpack_header_var(const char* data, unsigned dlen) {
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

	if (!this->unpack_short(&hbuf_[0], 2, pkt_id_)) {
		logger_error("unpack pkt id error");
		return -1;
	}
	finished_ = true;
	return dlen;
}

} // namespace acl
