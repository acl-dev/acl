#pragma once

#include "mqtt_message.hpp"

namespace acl {

class ACL_CPP_API mqtt_subscribe : public mqtt_message {
public:
	mqtt_subscribe(unsigned body_dlen = 0);
	~mqtt_subscribe(void);

	void set_pkt_id(unsigned short id);
	void add_topic(const char* topic, mqtt_qos_t qos);

	unsigned short get_pkt_id(void) const {
		return pkt_id_;
	}

	const std::vector<string>& get_topics(void) const {
		return topics_;
	}

	const std::vector<mqtt_qos_t>& get_qoses(void) const {
		return qoses_;
	}

protected:
	// @override
	bool to_string(string& out);

	// @override
	int update(const char* data, int dlen);

	// @override
	bool is_finished(void) const {
		return finished_;
	}

public:
	int update_header_var(const char* data, int dlen);
	int update_topic_len(const char* data, int dlen);
	int update_topic_val(const char* data, int dlen);
	int update_topic_qos(const char* data, int dlen);

protected:
	// @override
	unsigned char get_header_flags(void) const {
		return 0x02;
	}

private:
	bool finished_;
	char buff_[2];
	unsigned dlen_;

	unsigned short          pkt_id_;
	std::vector<string>     topics_;
	std::vector<mqtt_qos_t> qoses_;

	unsigned body_len_;
	unsigned nread_;

	string topic_;
};

} // namespace acl
