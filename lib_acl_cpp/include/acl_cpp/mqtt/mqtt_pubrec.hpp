#pragma once

#include "mqtt_ack.hpp"

namespace acl {

/**
 * mqtt message object for MQTT_PUBREC type.
 */
class ACL_CPP_API mqtt_pubrec : public mqtt_ack {
public:
	/**
	 * constructor for creating MQTT_PUBREC mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_pubrec(void);

	/**
	 * constructor for creating MQTT_PUBREC mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_pubrec(const mqtt_header& header);

	~mqtt_pubrec(void);
};

} // namespace acl
