#pragma once

#include "mqtt_message.hpp"

namespace acl {

class ACL_CPP_API mqtt_ack : public mqtt_message {
public:
	mqtt_ack(mqtt_type_t type);
	mqtt_ack(const mqtt_header& header);
	virtual ~mqtt_ack(void);

	void set_pkt_id(unsigned short id);

	unsigned short get_pkt_id(void) const {
		return pkt_id_;
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

private:
	unsigned status_;
	bool finished_;
	char hbuf_[2];
	unsigned hlen_;

	unsigned short pkt_id_;
};

} // namespace acl
