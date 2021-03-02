#include "acl_stdafx.hpp"
#include "acl_cpp/mqtt/mqtt_pubrel.hpp"

namespace acl {

mqtt_pubrel::mqtt_pubrel(void)
: mqtt_puback(MQTT_PUBREL)
{
}

mqtt_pubrel::~mqtt_pubrel(void) {}

} // namespace acl
