#pragma once

#include "mqtt_message.hpp"

namespace acl {

/**
 * the status of connection.
 */
enum {
	MQTT_CONNACK_OK       = 0x00,
	MQTT_CONNACK_ERR_VER  = 0x01,
	MQTT_CONNACK_ERR_CID  = 0x02,
	MQTT_CONNACK_ERR_SVR  = 0x03,
	MQTT_CONNACK_ERR_AUTH = 0x04,
	MQTT_CONNACK_ERR_DENY = 0x05,
};

/**
 * mqtt message object for the MQTT_CONNACK type.
 */
class ACL_CPP_API mqtt_connack : public mqtt_message {
public:
	/**
	 * constructor for creating MQTT_CONNACK mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_connack(void);

	/**
	 * constructor for creating MQTT_CONNACK mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_connack(const mqtt_header& header);

	~mqtt_connack(void);

	/**
	 * set the session control for the connection
	 * @param on {bool}
	 * @return {mqtt_connack&}
	 */
	mqtt_connack& set_session(bool on);

	/**
	 * set the connect return code
	 * @param code {unsigned char} defined as MQTT_CONNACK_XXX above.
	 * @return {mqtt_connack&}
	 */
	mqtt_connack& set_connack_code(unsigned char code);

	/**
	 * get the session control status
	 * @return {bool}
	 */
	bool get_session(void) const {
		return session_;
	}

	/**
	 * get the connect resturn code
	 * @return {unsigned char} defined as MQTT_CONNACK_XXX above.
	 */
	unsigned char get_connack_code(void) const {
		return connack_code_;
	}

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
	/**
	 * (internal) update mqtt header data for parsing mqtt data.
	 * @param data {const char*} the data to be parsed.
	 * @param dlen {int} the length of data.
	 * @return {int} return the length of the left data.
	 */
	int update_header_var(const char* data, int dlen);

private:
	unsigned status_;
	bool finished_;
	char buff_[2];
	int  dlen_;

	bool session_;
	unsigned char conn_flags_;
	unsigned char connack_code_;
};

} // namespace acl
