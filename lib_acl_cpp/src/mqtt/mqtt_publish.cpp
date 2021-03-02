#include "acl_stdafx.hpp"
#include "acl_cpp/mqtt/mqtt_publish.hpp"

namespace acl {

mqtt_publish::mqtt_publish(bool parse_payload /* true */)
: mqtt_message(MQTT_PUBLISH)
, finished_(false)
, parse_payload_(parse_payload)
, hlen_(0)
, dup_(false)
, qos_(MQTT_QOS0)
, retain_(false)
, pkt_id_(0)
, payload_len_(0)
{
	status_ = MQTT_STAT_HDR_VAR;  // just for update()
}

mqtt_publish::~mqtt_publish(void) {}

void mqtt_publish::set_dup(bool yes) {
	dup_ = yes;
}

void mqtt_publish::set_qos(mqtt_qos_t qos) {
	qos_ = qos;
}

void mqtt_publish::set_retain(bool yes) {
	retain_ = yes;
}

void mqtt_publish::set_topic(const char* topic) {
	topic_ = topic;
}

void mqtt_publish::set_pkt_id(unsigned short id) {
	pkt_id_ = id;
}

void mqtt_publish::set_payload(unsigned len, const char* data /* NULL */) {
	payload_len_ = len;
	if (data && payload_len_ > 0) {
		payload_.copy(data, len);
	}
}

unsigned char mqtt_publish::get_header_flags(void) const {
	unsigned char flags = qos_;
	if (qos_ != MQTT_QOS0 && dup_) {
		flags |= 0x8;
	}
	return flags;
}

bool mqtt_publish::to_string(string& out) {
	bool old_mode = out.get_bin();
	out.set_bin(true);

	unsigned len = 2 + (unsigned) topic_.size() + 2 + payload_len_;
	this->set_data_length(len);

	if (!this->pack_header(out)) {
		out.set_bin(old_mode);
		return false;
	}

	this->pack_add(topic_, out);
	this->pack_add((unsigned short) pkt_id_, out);

	if (payload_len_ > 0 && !payload_.empty()) {
		out.append(payload_, payload_len_);
	}

	out.set_bin(old_mode);
	return true;
}

enum {
	MQTT_STAT_HDR_PKTID = MQTT_STAT_HDR_END,
	MQTT_STAT_DONE,
};

static struct {
	int status;
	int (mqtt_publish::*handler)(const char*, unsigned);
} publish_handlers[] = {
	{ MQTT_STAT_STR_LEN,	&mqtt_message::unpack_string_len	},
	{ MQTT_STAT_STR_VAL,	&mqtt_message::unpack_string_val	},

	{ MQTT_STAT_HDR_VAR,	&mqtt_publish::unpack_header_var	},
	{ MQTT_STAT_HDR_PKTID,	&mqtt_publish::unpack_header_pktid	},
	{ MQTT_STAT_DONE,	&mqtt_publish::unpack_done		},
};

int mqtt_publish::update(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		logger_error("invalid input");
		return -1;
	}

	while (dlen > 0 && !finished_) {
		int ret = (this->*publish_handlers[status_].handler)(data, dlen);
		if (ret < 0) {
			return -1;
		}
		dlen = (unsigned) ret;
		data += ret;
	}
	return dlen;
}

int mqtt_publish::unpack_header_var(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		return 0;
	}

	int next = MQTT_STAT_HDR_PKTID;

	this->unpack_string_await(topic_, next);
	return dlen;
}

#define	HDR_PKTID_LEN	2

int mqtt_publish::unpack_header_pktid(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		return 0;
	}
	assert(sizeof(hbuf_) >= HDR_PKTID_LEN);

	if (hlen_ >= HDR_PKTID_LEN) {
		logger_error("invalid pkt id");
		return -1;
	}

	for (; hlen_ < HDR_PKTID_LEN && dlen > 0;) {
		hbuf_[hlen_++] = *data++;
		dlen--;
	}

	if (hlen_ < HDR_PKTID_LEN) {
		assert(dlen == 0);
		return dlen;
	}

	if (!this->unpack_short(&hbuf_[0], 2, pkt_id_)) {
		logger_error("unpack pkt_id error");
		return -1;
	}

	if (parse_payload_) {
		int next = MQTT_STAT_DONE;
		this->unpack_string_await(payload_, next);
	} else {
		finished_ = true;
	}
	return dlen;
}

int mqtt_publish::unpack_done(const char*, unsigned dlen) {
	finished_ = true;
	return dlen;
}

} // namespace acl
