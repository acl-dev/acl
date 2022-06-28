#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_unsuback.hpp"
#endif

namespace acl {

mqtt_unsuback::mqtt_unsuback(void)
: mqtt_ack(MQTT_PUBACK)
{
}

mqtt_unsuback::mqtt_unsuback(const mqtt_header& header)
: mqtt_ack(header)
{
}

mqtt_unsuback::~mqtt_unsuback(void) {}

} // namespace acl
