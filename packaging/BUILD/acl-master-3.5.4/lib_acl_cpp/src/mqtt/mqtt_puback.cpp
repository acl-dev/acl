#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_puback.hpp"
#endif

namespace acl {

mqtt_puback::mqtt_puback(void)
: mqtt_ack(MQTT_PUBACK)
{
}

mqtt_puback::mqtt_puback(const mqtt_header& header)
: mqtt_ack(header)
{
}

mqtt_puback::~mqtt_puback(void) {}

} // namespace acl
