#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_unsubscribe.hpp"
#endif

namespace acl {

enum {
	MQTT_STAT_HDR_VAR,
	MQTT_STAT_TOPIC_LEN,
	MQTT_STAT_TOPIC_VAL,
};

mqtt_unsubscribe::mqtt_unsubscribe(void)
: mqtt_message(MQTT_UNSUBSCRIBE)
, finished_(false)
, dlen_(0)
, pkt_id_(0)
, body_len_(0)
, nread_(0)
{
	status_ = MQTT_STAT_HDR_VAR; /* just for update */
}

mqtt_unsubscribe::mqtt_unsubscribe(const mqtt_header& header)
: mqtt_message(header)
, finished_(false)
, dlen_(0)
, pkt_id_(0)
, nread_(0)
{
	status_   = MQTT_STAT_HDR_VAR; /* just for update */
	body_len_ = header.get_remaining_length();
}

mqtt_unsubscribe::~mqtt_unsubscribe(void) {}

mqtt_unsubscribe& mqtt_unsubscribe::set_pkt_id(unsigned short id) {
	if (id > 0) {
		pkt_id_ = id;
	} else {
		logger_error("invalid id=%u", id);
	}
	return *this;
}

mqtt_unsubscribe& mqtt_unsubscribe::add_topic(const char* topic) {
	topics_.push_back(topic);
	body_len_ += (unsigned) strlen(topic);
	return *this;
}

bool mqtt_unsubscribe::to_string(string& out) {
	if (pkt_id_ == 0) {
		logger_error("pkt_id=0 invalid");
		return false;
	}
	if (topics_.empty()) {
		logger_error("no topic available!");
		return false;
	}

	body_len_ += sizeof(pkt_id_);

	mqtt_header& header = this->get_header();
	header.set_remaing_length(body_len_);

	if (!header.build_header(out)) {
		return false;
	}

	this->pack_add((unsigned short) pkt_id_, out);

	size_t n = topics_.size();
	for (size_t i = 0; i < n; i++) {
		this->pack_add(topics_[i], out);
	}

	return true;
}

static struct {
	int status;
	int (mqtt_unsubscribe::*handler)(const char*, int);
} handlers[] = {
	{ MQTT_STAT_HDR_VAR,	&mqtt_unsubscribe::update_header_var	},

	{ MQTT_STAT_TOPIC_LEN,	&mqtt_unsubscribe::update_topic_len	},
	{ MQTT_STAT_TOPIC_VAL,	&mqtt_unsubscribe::update_topic_val	},
};

int mqtt_unsubscribe::update(const char* data, int dlen) {
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

int mqtt_unsubscribe::update_header_var(const char* data, int dlen) {
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

	if (nread_ >= body_len_) {
		logger_error("no payload!");
		return -1;
	}

	dlen_   = 0;
	status_ = MQTT_STAT_TOPIC_LEN;

	return dlen;
}

#define	HDR_LEN_LEN	2

int mqtt_unsubscribe::update_topic_len(const char* data, int dlen) {
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

	if (nread_ >= body_len_) {
		logger_error("overflow, nread=%u, body_len=%u",
			nread_, body_len_);
		return -1;
	}

	dlen_   = n;
	status_ = MQTT_STAT_TOPIC_VAL;

	return dlen;
}

int mqtt_unsubscribe::update_topic_val(const char* data, int dlen) {
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

	nread_ += (unsigned) topic_.size();

	if (nread_ >= body_len_) {
		finished_ = true;
		return dlen;
	}

	dlen_   = 0;
	status_ = MQTT_STAT_TOPIC_LEN;

	return dlen;
}

} // namespace acl
