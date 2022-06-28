#pragma once

#include "mqtt_ack.hpp"

namespace acl {

/**
 * mqtt message object for MQTT_PUBCOMP type.
 */
class ACL_CPP_API mqtt_pubcomp : public mqtt_ack {
public:
	/**
	 * constructor for creating MQTT_PUBCOMP mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_pubcomp(void);

	/**
	 * constructor for creating MQTT_PUBCOMP mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_pubcomp(const mqtt_header& header);

	~mqtt_pubcomp(void);
};

} // namespace acl
