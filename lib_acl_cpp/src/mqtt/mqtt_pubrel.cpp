#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_pubrel.hpp"
#endif

namespace acl {

mqtt_pubrel::mqtt_pubrel(void)
: mqtt_ack(MQTT_PUBREL)
{
}

mqtt_pubrel::~mqtt_pubrel(void) {}

} // namespace acl
