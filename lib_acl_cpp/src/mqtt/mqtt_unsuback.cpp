#include "acl_stdafx.hpp"
#include "acl_cpp/mqtt/mqtt_unsuback.hpp"

namespace acl {

mqtt_unsuback::mqtt_unsuback()
: mqtt_ack(MQTT_PUBACK)
{
}

mqtt_unsuback::~mqtt_unsuback(void) {}

} // namespace acl
