#pragma once

#include "mqtt_message.hpp"

namespace acl {

enum {
	MQTT_CONNACK_OK       = 0x00,
	MQTT_CONNACK_ERR_VER  = 0x01,
	MQTT_CONNACK_ERR_CID  = 0x02,
	MQTT_CONNACK_ERR_SVR  = 0x03,
	MQTT_CONNACK_ERR_AUTH = 0x04,
	MQTT_CONNACK_ERR_DENY = 0x05,
};

class ACL_CPP_API mqtt_connack : public mqtt_message {
public:
	mqtt_connack(void);
	~mqtt_connack(void);

	void set_session(bool on);
	void set_connack_code(unsigned char code);

	bool get_session(void) const {
		return session_;
	}

	unsigned char get_connack_code(void) const {
		return connack_code_;
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
	bool finished_;
	char buff_[2];
	int  dlen_;

	bool session_;
	unsigned char conn_flags_;
	unsigned char connack_code_;
};

} // namespace acl
