#pragma once

#include "mqtt_ack.hpp"

namespace acl {

/**
 * mqtt message object for MQTT_PUBREL type.
 */
class ACL_CPP_API mqtt_pubrel : public mqtt_ack {
public:
	/**
	 * constructor for creating MQTT_PUBREL mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_pubrel(void);

	/**
	 * constructor for creating MQTT_PUBREL mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_pubrel(const mqtt_header& header);

	~mqtt_pubrel(void);
};

} // namespace acl
