#pragma once

#include "mqtt_message.hpp"

namespace acl {

class mqtt_publish : public mqtt_message {
public:
	mqtt_publish(unsigned payload_len);
	~mqtt_publish(void);

	void set_dup(bool yes);
	void set_qos(mqtt_qos_t qos);
	void set_retain(bool yes);
	void set_topic(const char* topic);
	void set_pkt_id(unsigned short id);
	void set_payload(unsigned len, const char* data = NULL);

	bool is_dup(void) const {
		return qos_ != MQTT_QOS0 && dup_;
	}

	mqtt_qos_t get_qos(void) const {
		return qos_;
	}

	bool is_retain(void) const {
		return retain_;
	}


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

	int update(const char* data, int dlen);

	bool is_finished(void) const {
		return finished_;
	}

	const string& get_payload(void) const {
		return payload_;
	}

public:
	int update_header_var(const char* data, int dlen);
	int update_topic_len(const char* data, int dlen);
	int update_topic_val(const char* data, int dlen);
	int update_header_pktid(const char* data, int dlen);
	int update_payload(const char* data, int dlen);

protected:
	// @override
	unsigned char get_header_flags(void) const;

private:
	bool finished_;
	char buff_[2];
	int  dlen_;
	unsigned hlen_var_;

	bool dup_;
	mqtt_qos_t qos_;
	bool retain_;
	string topic_;
	unsigned short pkt_id_;
	unsigned payload_len_;
	string payload_;
};

} // namespace acl
