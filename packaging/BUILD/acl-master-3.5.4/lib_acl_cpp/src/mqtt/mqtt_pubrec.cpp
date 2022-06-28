#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_pubrec.hpp"
#endif

namespace acl {

mqtt_pubrec::mqtt_pubrec(void)
: mqtt_ack(MQTT_PUBREC)
{
}

mqtt_pubrec::mqtt_pubrec(const mqtt_header& header)
: mqtt_ack(header)
{
}

mqtt_pubrec::~mqtt_pubrec(void) {}

} // namespace acl
