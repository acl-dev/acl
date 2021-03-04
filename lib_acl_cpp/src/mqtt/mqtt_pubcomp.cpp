#include "acl_stdafx.hpp"
#include "acl_cpp/mqtt/mqtt_pubcomp.hpp"

namespace acl {

mqtt_pubcomp::mqtt_pubcomp(void)
: mqtt_ack(MQTT_PUBCOMP)
{
}

mqtt_pubcomp::~mqtt_pubcomp(void) {}

} // namespace acl
