#pragma once

#include "mqtt_message.hpp"

namespace acl {

/**
 * mqtt message object for the MQTT_DISCONNECT type.
 */
class ACL_CPP_API mqtt_disconnect : public mqtt_message {
public:
	/**
	 * constructor for creating MQTT_DISCONNECT mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_disconnect(void);

	/**
	 * constructor for creating MQTT_DISCONNECT mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_disconnect(const mqtt_header& header);

	~mqtt_disconnect(void);

protected:
	// @override
	bool to_string(string& out);

	// @override
	int update(const char*, int dlen) {
		return dlen;
	}

	// @override
	bool finished(void) const {
		return true;
	}
};

} // namespace acl
