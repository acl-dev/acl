#pragma once

#include "mqtt_message.hpp"

namespace acl {

class mqtt_publish : public mqtt_message {
public:
	mqtt_publish(bool parse_payload = true);
	~mqtt_publish(void);

	void set_topic(const char* topic);
	void set_pkt_id(unsigned short id);
	void set_payload(unsigned len, const char* data = NULL);

	const char* get_topic(void) const {
		return topic_.c_str();
	}

	unsigned short get_pkt_id(void) const {
		return pkt_id_;
	}

	unsigned get_payload_len(void) const {
		return payload_len_;
	}

	bool to_string(string& out);

	int update(const char* data, unsigned dlen);

	bool is_finished(void) const {
		return finished_;
	}

public:
	int unpack_header_var(const char* data, unsigned dlen);
	int unpack_header_pktid(const char* data, unsigned dlen);
	int unpack_done(const char* data, unsigned dlen);

private:
	bool finished_;
	bool parse_payload_;
	char hbuf_[2];
	unsigned hlen_;

	string topic_;
	unsigned short pkt_id_;
	unsigned payload_len_;
	string payload_;
};

} // namespace acl
