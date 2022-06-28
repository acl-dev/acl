#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_pubcomp.hpp"
#endif

namespace acl {

mqtt_pubcomp::mqtt_pubcomp(void)
: mqtt_ack(MQTT_PUBCOMP)
{
}

mqtt_pubcomp::mqtt_pubcomp(const mqtt_header& header)
: mqtt_ack(header)
{
}

mqtt_pubcomp::~mqtt_pubcomp(void) {}

} // namespace acl
