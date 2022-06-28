#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_publish.hpp"
#endif

namespace acl {

enum {
	MQTT_STAT_HDR_VAR,
	MQTT_STAT_TOPIC_LEN,
	MQTT_STAT_TOPIC_VAL,
	MQTT_STAT_PKTID,
	MQTT_STAT_PAYLOAD,
};

mqtt_publish::mqtt_publish(void)
: mqtt_message(MQTT_PUBLISH)
, finished_(false)
, dlen_(0)
, hlen_var_(0)
, pkt_id_(0)
, payload_len_(0)
{
	status_ = MQTT_STAT_HDR_VAR;  // just for update()
}

mqtt_publish::mqtt_publish(const mqtt_header& header)
: mqtt_message(header)
, finished_(false)
, dlen_(0)
, hlen_var_(0)
, pkt_id_(0)
{
	status_      = MQTT_STAT_HDR_VAR;  // just for update()
	payload_len_ = header.get_remaining_length();
}

mqtt_publish::~mqtt_publish(void) {}

mqtt_publish& mqtt_publish::set_topic(const char* topic) {
	topic_ = topic;
	return *this;
}

mqtt_publish& mqtt_publish::set_pkt_id(unsigned short id) {
	if (id > 0) {
		pkt_id_ = id;
	} else {
		logger_warn("pkt id should > 0, id=%d", id);
	}
	return *this;
}

mqtt_publish& mqtt_publish::set_payload(unsigned len, const char* data /* NULL */) {
	payload_len_ = len;
	if (data && payload_len_ > 0) {
		payload_.copy(data, len);
	}
	return *this;
}

bool mqtt_publish::to_string(string& out) {
	if (pkt_id_ == 0) {
		logger_error("pkt_id should > 0, pkt_id=%u", pkt_id_);
		return false;
	}

	unsigned len = (unsigned) topic_.size() + 2 + payload_len_;
	mqtt_qos_t qos = this->get_header().get_qos();
	if (qos != MQTT_QOS0) {
		len += 2;
	}

	mqtt_header& header = this->get_header();

	if (header.is_dup() && header.get_qos() == MQTT_QOS0) {
		header.set_dup(false);
	}

	header.set_remaing_length(len);

	if (!header.build_header(out)) {
		logger_error("build header error");
		return false;
	}

	this->pack_add(topic_, out);

	if (qos != MQTT_QOS0) {
		this->pack_add((unsigned short) pkt_id_, out);
	}

	if (payload_len_ > 0 && !payload_.empty()) {
		out.append(payload_, payload_len_);
	}

	return true;
}

static struct {
	int status;
	int (mqtt_publish::*handler)(const char*, int);
} handlers[] = {
	{ MQTT_STAT_HDR_VAR,	&mqtt_publish::update_header_var },

	{ MQTT_STAT_TOPIC_LEN,	&mqtt_publish::update_topic_len  },
	{ MQTT_STAT_TOPIC_VAL,	&mqtt_publish::update_topic_val  },
	{ MQTT_STAT_PKTID,	&mqtt_publish::update_pktid      },
	{ MQTT_STAT_PAYLOAD,	&mqtt_publish::update_payload    },
};

int mqtt_publish::update(const char* data, int dlen) {
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

int mqtt_publish::update_header_var(const char* data, int dlen) {
	(void) data;
	assert(data && dlen > 0);

	dlen_   = 0;
	status_ = MQTT_STAT_TOPIC_LEN;
	return dlen;
}

#define	HDR_LEN_LEN	2

int mqtt_publish::update_topic_len(const char* data, int dlen) {
	assert(data && dlen > 0);
	assert(sizeof(buff_) >= HDR_LEN_LEN);

	for (; dlen_ < HDR_LEN_LEN && dlen > 0;) {
		buff_[dlen_++] = *data++;
		dlen--;
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

	dlen_     = n;
	hlen_var_ = n + HDR_LEN_LEN;
	status_   = MQTT_STAT_TOPIC_VAL;

	return dlen;
}

int mqtt_publish::update_topic_val(const char* data, int dlen) {
	assert(data && dlen > 0 && dlen_ > 0);

	for (; dlen_ > 0 && dlen > 0;) {
		topic_ += *data++;
		--dlen_;
		--dlen;
	}

	if (dlen_ > 0) {
		assert(dlen == 0);
		return dlen;
	} 

	dlen_   = 0;
	if (this->get_header().get_qos() != MQTT_QOS0) {
		status_ = MQTT_STAT_PKTID;
	} else {
		payload_len_ -= hlen_var_;
		status_ = MQTT_STAT_PAYLOAD;
	}

	return dlen;
}

#define	HDR_PKTID_LEN	2

int mqtt_publish::update_pktid(const char* data, int dlen) {
	assert(data && dlen > 0);
	assert(sizeof(buff_) >= HDR_PKTID_LEN);

	if (dlen_ >= HDR_PKTID_LEN) {
		logger_error("invalid pkt id");
		return -1;
	}

	for (; dlen_ < HDR_PKTID_LEN && dlen > 0;) {
		buff_[dlen_++] = *data++;
		dlen--;
	}

	if (dlen_ < HDR_PKTID_LEN) {
		assert(dlen == 0);
		return dlen;
	}

	if (!this->unpack_short(&buff_[0], 2, pkt_id_)) {
		logger_error("unpack pkt_id error");
		return -1;
	}

	hlen_var_ += HDR_PKTID_LEN;

	if (payload_len_ == 0) {
		finished_ = true;
		return dlen;
	}

	if (payload_len_ < hlen_var_) {
		logger_error("invalid payload len=%u, hlen_var=%u",
			payload_len_, hlen_var_);
		return -1;
	}

	payload_len_ -= hlen_var_;
	if (payload_len_ == 0) {
		finished_ = true;
		return dlen;
	}

	status_ = MQTT_STAT_PAYLOAD;
	return dlen;
}

int mqtt_publish::update_payload(const char* data, int dlen) {
	assert(data && dlen > 0);
	assert((size_t) payload_len_ > payload_.size());

	size_t i, left = (size_t) payload_len_ - payload_.size();
	for (i = 0; i < left && dlen > 0; i++) {
		payload_ += *data++;
		dlen--;
	}

	if (i == left) {
		finished_ = true;
	}

	return dlen;
}

} // namespace acl
