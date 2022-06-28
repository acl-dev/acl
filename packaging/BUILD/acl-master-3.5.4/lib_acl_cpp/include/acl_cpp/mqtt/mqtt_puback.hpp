#pragma once

#include "mqtt_ack.hpp"

namespace acl {

/**
 * mqtt message object for MQTT_PUBACK type.
 */
class ACL_CPP_API mqtt_puback : public mqtt_ack {
public:
	/**
	 * constructor for creating MQTT_PUBACK mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_puback(void);

	/**
	 * constructor for creating MQTT_PUBACK mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_puback(const mqtt_header& header);

	~mqtt_puback(void);
};

} // namespace acl
