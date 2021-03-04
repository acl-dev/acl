#pragma once

#include "mqtt_message.hpp"

namespace acl {

class mqtt_unsubscribe : public mqtt_message {
public:
	mqtt_unsubscribe(unsigned payload_len = 0);
	~mqtt_unsubscribe(void);

	void set_pkt_id(unsigned short id);
	void add_topic(const char* topic);

	bool to_string(string& out);

	int update(const char* data, int dlen);

public:
	int update_header_var(const char* data, int dlen);
	int update_topic_len(const char* data, int dlen);
	int update_topic_val(const char* data, int dlen);

private:
	bool finished_;
	char buff_[2];
	unsigned dlen_;

	unsigned short  pkt_id_;
	std::vector<string> topics_;

	unsigned payload_len_;
	unsigned nread_;

	string topic_;
};

} // namespace acl

