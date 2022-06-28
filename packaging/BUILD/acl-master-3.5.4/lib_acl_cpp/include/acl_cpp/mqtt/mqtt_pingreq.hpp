#pragma once

#include "mqtt_message.hpp"

namespace acl {

/**
 * mqtt message object for MQTT_PINGREQ type.
 */
class ACL_CPP_API mqtt_pingreq : public mqtt_message {
public:
	/**
	 * constructor for creating MQTT_PINGREQ mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_pingreq(void);

	/**
	 * constructor for creating MQTT_PINGREQ mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_pingreq(const mqtt_header& header);

	~mqtt_pingreq(void);

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
