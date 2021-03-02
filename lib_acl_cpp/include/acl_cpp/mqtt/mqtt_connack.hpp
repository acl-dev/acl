#pragma once

#include "mqtt_message.hpp"

namespace acl {

class mqtt_connack : public mqtt_message {
public:
	mqtt_connack(void);
	~mqtt_connack(void);

	void set_session(bool on);
	bool get_session(void) const {
		return session_;
	}

	int update(const char* data, unsigned dlen);

public:
	int unpack_header_var(const char* data, unsigned dlen);

private:
	bool finished_;
	char hbuf_[2];
	unsigned hlen_;

	bool session_;
	unsigned char conn_flags_;
};

} // namespace acl
