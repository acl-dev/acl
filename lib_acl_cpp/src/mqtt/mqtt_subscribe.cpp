#include "acl_stdafx.hpp"
#include "acl_cpp/mqtt/mqtt_subscribe.hpp"

namespace acl {

mqtt_subscribe::mqtt_subscribe(unsigned payload_len /* 0 */)
: mqtt_message(MQTT_SUBSCRIBE)
, finished_(false)
, pkt_id_(0)
, payload_len_(payload_len)
, nread_(0)
{
}

mqtt_subscribe::~mqtt_subscribe(void) {}

void mqtt_subscribe::set_pkt_id(unsigned short id) {
	pkt_id_ = id;
}

void mqtt_subscribe::add_topic(const char* topic, mqtt_qos_t qos) {
	topics_.push_back(topic);
	qoses_.push_back(qos);
	payload_len_ += strlen(topic) + 1;
}

bool mqtt_subscribe::to_string(string& out) {
	if (topics_.empty()) {
		logger_error("no topic available!");
		return false;
	}

	bool old_mode = out.get_bin();
	this->set_payload_length(payload_len_);

	if (!this->pack_header(out)) {
		out.set_bin(old_mode);
		return false;
	}

	size_t n = topics_.size();
	for (size_t i = 0; i < n; i++) {
		this->pack_add(topics_[i], out);
		this->pack_add((unsigned char) qoses_[i], out);
	}

	out.set_bin(old_mode);
	return true;
}

enum {
	MQTT_STAT_TOPIC_DONE = MQTT_STAT_HDR_END,
};

static struct {
	int status;
	int (mqtt_subscribe::*handler)(const char*, unsigned);
} handlers[] = {
	{ MQTT_STAT_STR_LEN,	&mqtt_message::unpack_string_len	},
	{ MQTT_STAT_STR_VAL,	&mqtt_message::unpack_string_val	},

	{ MQTT_STAT_HDR_VAR,	&mqtt_subscribe::unpack_header_var	},
	{ MQTT_STAT_TOPIC_DONE,	&mqtt_subscribe::unpack_topic_done	},
};

int mqtt_subscribe::update(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		return 0;
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
int mqtt_subscribe::unpack_header_var(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		return 0;
	}

	int next = MQTT_STAT_TOPIC_DONE;
	this->unpack_string_await(topic_, next);
	return dlen;
}

int mqtt_subscribe::unpack_topic_done(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		return 0;
	}

	if (topic_.empty()) {
		logger_error("no topic got");
		return -1;
	}

	char qos = *data++;
	dlen--;
	if (qos < (char) MQTT_QOS0 || qos > (char) MQTT_QOS2) {
		logger_warn("invalid qos=%d, topic=%s", qos, topic_.c_str());
		qos = MQTT_QOS0;
	}

	topics_.push_back(topic_);
	qoses_.push_back((mqtt_qos_t) qos);

	nread_ += (unsigned) topic_.size() + 1;
	if (nread_ >= payload_len_) {
		finished_ = true;
		return dlen;
	}

	int next = MQTT_STAT_HDR_VAR;
	this->unpack_string_await(topic_, next);
	return dlen;
}

} // namespace acl
