#include "acl_stdafx.hpp"
#include "acl_cpp/mqtt/mqtt_subscribe.hpp"

namespace acl {

enum {
	MQTT_STAT_HDR_VAR,
	MQTT_STAT_TOPIC_LEN,
	MQTT_STAT_TOPIC_VAL,
	MQTT_STAT_TOPIC_QOS,
};

mqtt_subscribe::mqtt_subscribe(unsigned payload_len /* 0 */)
: mqtt_message(MQTT_SUBSCRIBE)
, finished_(false)
, dlen_(0)
, pkt_id_(0)
, payload_len_(payload_len)
, nread_(0)
{
	status_ = MQTT_STAT_HDR_VAR; /* just for update */
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

	payload_len_ += sizeof(pkt_id_);
	this->set_data_length(payload_len_);

	if (!this->pack_header(out)) {
		out.set_bin(old_mode);
		return false;
	}

	this->pack_add((unsigned short) pkt_id_, out);

	size_t n = topics_.size();
	for (size_t i = 0; i < n; i++) {
		this->pack_add(topics_[i], out);
		this->pack_add((unsigned char) qoses_[i], out);
	}

	out.set_bin(old_mode);
	return true;
}

static struct {
	int status;
	int (mqtt_subscribe::*handler)(const char*, int);
} handlers[] = {
	{ MQTT_STAT_HDR_VAR,	&mqtt_subscribe::update_header_var	},

	{ MQTT_STAT_TOPIC_LEN,	&mqtt_subscribe::update_topic_len	},
	{ MQTT_STAT_TOPIC_VAL,	&mqtt_subscribe::update_topic_val	},
	{ MQTT_STAT_TOPIC_QOS,	&mqtt_subscribe::update_topic_qos	},
};

int mqtt_subscribe::update(const char* data, int dlen) {
	if (data == NULL || dlen <= 0) {
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

int mqtt_subscribe::update_header_var(const char* data, int dlen) {
	assert(data && dlen > 0);
	assert(sizeof(buff_) >= HDR_VAR_LEN);

	if (nread_ >= HDR_VAR_LEN) {
		logger_error("invalid header var");
		return -1;
	}

	for (; nread_ < HDR_VAR_LEN && dlen > 0;) {
		buff_[nread_++] = *data++;
		dlen--;
	}

	if (nread_ < HDR_VAR_LEN) {
		assert(dlen == 0);
		return dlen;
	}

	if (!this->unpack_short(&buff_[0], 2, pkt_id_)) {
		logger_error("unpack pkt id error");
		return -1;
	}

	if (nread_ >= payload_len_) {
		logger_warn("no payload!");
		finished_ = true;
		return dlen;
	}

	dlen_   = 0;
	status_ = MQTT_STAT_TOPIC_LEN;

	return dlen;
}

#define	HDR_LEN_LEN	2

int mqtt_subscribe::update_topic_len(const char* data, int dlen) {
	assert(data && dlen > 0);
	assert(sizeof(buff_) >= HDR_LEN_LEN);

	for (; dlen_ < HDR_LEN_LEN && dlen > 0;) {
		buff_[dlen_++] = *data++;
		dlen--;
		nread_++;
	}

	if (dlen_ < HDR_LEN_LEN) {
		assert(dlen == 0);
		return dlen;
	}

	unsigned short n;
	if (!this->unpack_short(&buff_[0], 2, n)) {
		logger_error("unpack cid len error");
		return -1;
	}

	if (n == 0) {
		logger_error("invalid topic len");
		return -1;
	}

	if (nread_ >= payload_len_) {
		logger_error("overflow, nread=%u, payload_len=%u",
			nread_, payload_len_);
		return -1;
	}

	dlen_   = n;
	status_ = MQTT_STAT_TOPIC_VAL;

	return dlen;
}

int mqtt_subscribe::update_topic_val(const char* data, int dlen) {
	assert(data && dlen > 0 && dlen_ > 0);

	for (; dlen_ > 0 && dlen > 0;) {
		topic_ += data++;
		--dlen_;
		--dlen;
	}

	if (dlen_ > 0) {
		assert(dlen == 0);
		return dlen;
	} 

	nread_ += (unsigned) topic_.size();
	dlen_   = 0;
	status_ = MQTT_STAT_TOPIC_QOS;

	return dlen;
}

int mqtt_subscribe::update_topic_qos(const char* data, int dlen) {
	assert(data && dlen > 0);

	if (topic_.empty()) {
		logger_error("no topic got");
		return -1;
	}

	char qos = *data++;
	dlen--;
	nread_++;

	if (qos < (char) MQTT_QOS0 || qos > (char) MQTT_QOS2) {
		logger_warn("invalid qos=%d, topic=%s", qos, topic_.c_str());
		qos = MQTT_QOS0;
	}

	topics_.push_back(topic_);
	topic_.clear();

	qoses_.push_back((mqtt_qos_t) qos);

	if (nread_ >= payload_len_) {
		finished_ = true;
		return dlen;
	}

	dlen_   = 0;
	status_ = MQTT_STAT_TOPIC_LEN;

	return dlen;
}

} // namespace acl
