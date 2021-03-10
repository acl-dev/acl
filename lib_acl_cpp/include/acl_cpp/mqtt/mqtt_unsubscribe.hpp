#pragma once

#include "mqtt_message.hpp"

namespace acl {

class ACL_CPP_API mqtt_unsubscribe : public mqtt_message {
public:
	mqtt_unsubscribe(void);
	mqtt_unsubscribe(const mqtt_header& header);
	~mqtt_unsubscribe(void);

	void set_pkt_id(unsigned short id);
	void add_topic(const char* topic);

protected:
	// @override
	bool to_string(string& out);

	// @override
	int update(const char* data, int dlen);

	// @override
	bool finished(void) const {
		return finished_;
	}

public:
	int update_header_var(const char* data, int dlen);
	int update_topic_len(const char* data, int dlen);
	int update_topic_val(const char* data, int dlen);

private:
	unsigned status_;
	bool finished_;
	char buff_[2];
	unsigned dlen_;

	unsigned short  pkt_id_;
	std::vector<string> topics_;

	unsigned body_len_;
	unsigned nread_;

	string topic_;
};

} // namespace acl

