#include "acl_stdafx.hpp"
#include "acl_cpp/mqtt/mqtt_subscribe.hpp"

namespace acl {

mqtt_subscribe::mqtt_subscribe(void)
: mqtt_message(MQTT_SUBSCRIBE)
, pkt_id_(0)
, payload_len_(0)
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
	this->set_data_length(payload_len_);

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

int mqtt_subscribe::update(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		return 0;
	}

	return dlen;
}

} // namespace acl
