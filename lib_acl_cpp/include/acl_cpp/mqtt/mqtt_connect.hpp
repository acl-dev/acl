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

/**
 * mqtt message object for the MQTT_CONNECT type.
 */
class ACL_CPP_API mqtt_connect : public mqtt_message {
public:
	/**
	 * constructor for creating MQTT_CONNECT mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_connect(void);

	/**
	 * constructor for creating MQTT_CONNECT mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_connect(const mqtt_header& header);

	~mqtt_connect(void);

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
	void set_keep_alive(unsigned short keep_alive);
	void set_cid(const char* cid);
	void set_username(const char* name);
	void set_passwd(const char* passwd);
	void set_will_qos(mqtt_qos_t qos);
	void set_will_topic(const char* topic);
	void set_will_msg(const char* msg);
	void clean_session(void);

	unsigned short get_keep_alive(void) const {
		return keep_alive_;
	}

	const char* get_cid(void) const {
		return cid_.empty() ? NULL : cid_.c_str();;
	}

	const char* get_username(void) const {
		return username_.empty() ? NULL : username_.c_str();
	}

	const char* get_passwd(void) const {
		return passwd_.empty() ? NULL : passwd_.c_str();
	}

	mqtt_qos_t get_will_qos(void) const {
		return will_qos_;
	}

	const char* get_will_topic(void) const {
		return will_topic_.empty() ? NULL : will_topic_.c_str();
	}

	const char* get_will_msg(void) const {
		return will_msg_.empty() ? NULL : will_msg_.c_str();
	}

	bool has_session(void) const;

private:
	unsigned status_;
	bool finished_;
	char buff_[10];
	int  dlen_;

	mqtt_qos_t will_qos_;
	unsigned char  conn_flags_;
	unsigned short keep_alive_;

	string cid_;
	string username_;
	string passwd_;
	string will_topic_;
	string will_msg_;

public:
	// (internal)
	// the methods below will be used to update mqtt message object when
	// parsing mqtt body data.
	int update_header_var(const char* data, int dlen);
	int update_cid_len(const char* data, int dlen);
	int update_cid_val(const char* data, int dlen);
	int update_username_len(const char* data, int dlen);
	int update_username_val(const char* data, int dlen);
	int update_passwd_len(const char* data, int dlen);
	int update_passwd_val(const char* data, int dlen);
	int update_will_topic_len(const char* data, int dlen);
	int update_will_topic_val(const char* data, int dlen);
	int update_will_msg_len(const char* data, int dlen);
	int update_will_msg_val(const char* data, int dlen);
};

} // namespace acl
