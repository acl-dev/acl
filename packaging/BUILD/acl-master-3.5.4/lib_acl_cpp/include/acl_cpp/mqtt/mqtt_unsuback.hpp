#pragma once

#include "mqtt_ack.hpp"

namespace acl {

/**
 * mqtt message object for MQTT_UNPUBACK type.
 */
class ACL_CPP_API mqtt_unsuback : public mqtt_ack {
public:
	/**
	 * constructor for creating MQTT_UNPUBACK mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_unsuback(void);

	/**
	 * constructor for creating MQTT_UNPUBACK mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_unsuback(const mqtt_header& header);

	~mqtt_unsuback(void);
};

} // namespace acl
