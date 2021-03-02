#pragma once

#include "mqtt_message.hpp"

namespace acl {

class mqtt_subscribe : public mqtt_message {
public:
	mqtt_subscribe(void);
	~mqtt_subscribe(void);

	void set_pkt_id(unsigned short id);
	void add_topic(const char* topic, mqtt_qos_t qos);

	unsigned short get_pkt_id(void) const {
		return pkt_id_;
	}

	bool to_string(string& out);

	int update(const char* data, unsigned dlen);

protected:
	// @override
	unsigned char get_header_flags(void) const {
		return 0x02;
	}

private:
	unsigned short          pkt_id_;
	std::vector<string>     topics_;
	std::vector<mqtt_qos_t> qoses_;
	unsigned payload_len_;
};

} // namespace acl
