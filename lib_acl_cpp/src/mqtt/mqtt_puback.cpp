#include "acl_stdafx.hpp"
#include "acl_cpp/mqtt/mqtt_puback.hpp"

namespace acl {

mqtt_puback::mqtt_puback()
: mqtt_ack(MQTT_PUBACK)
{
}

mqtt_puback::~mqtt_puback(void) {}

} // namespace acl
