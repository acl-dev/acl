#pragma once

#include "mqtt_message.hpp"

namespace acl {

typedef enum {
	CONNECT_ACCEPTED	= 0x00,
	CONNECT_INVALID_VERSION	= 0x01,
	CONNECT_INVALID_CID	= 0x02,
	CONNECT_NOT_AVAIL	= 0x03,
	CONNECT_LOGIN_FAILED	= 0x04,
	CONNECT_NO_AUTHORITY	= 0x05,
} mqtt_conn_status_t;

class mqtt_connect : public mqtt_message {
public:
	mqtt_connect(void);
	~mqtt_connect(void);

	void set_keep_alive(unsigned short keep_alive);
	void set_username(const char* name);
	void set_passwd(const char* passwd);
	void set_qos(mqtt_qos_t qos);
	void set_will_topic(const char* topic);
	void set_will_msg(const char* msg);

	bool to_string(string& out);

	int update(const char* data, unsigned dlen);

	bool is_finished(void) const {
		return finished_;
	}

	unsigned short get_keep_alive(void) const {
		return keep_alive_;
	}

	const char* get_username(void) const {
		return username_.empty() ? NULL : username_.c_str();
	}

	const char* get_passwd(void) const {
		return passwd_.empty() ? NULL : passwd_.c_str();
	}

	mqtt_qos_t get_qos(void) const {
		return qos_;
	}

	const char* get_will_topic(void) const {
		return will_topic_.empty() ? NULL : will_topic_.c_str();
	}

	const char* get_will_msg(void) const {
		return will_msg_.empty() ? NULL : will_msg_.c_str();
	}

private:
	bool finished_;
	char hbuf_[10];
	unsigned hlen_;

	mqtt_qos_t qos_;
	unsigned char conn_flags_;
	unsigned short keep_alive_;

	string cid_;
	string username_;
	string passwd_;
	string will_topic_;
	string will_msg_;

public:
	int unpack_header_var(const char* data, unsigned dlen);
	int unpack_cid(const char* data, unsigned dlen);
	int unpack_username(const char* data, unsigned dlen);
	int unpack_passwd(const char* data, unsigned dlen);
	int unpack_will_topic(const char* data, unsigned dlen);
	int unpack_will_msg(const char* data, unsigned dlen);
};

} // namespace acl
