#include "acl_stdafx.hpp"
#include "acl_cpp/mqtt/mqtt_pubrec.hpp"

namespace acl {

mqtt_pubrec::mqtt_pubrec(void)
: mqtt_ack(MQTT_PUBREC)
{
}

mqtt_pubrec::~mqtt_pubrec(void) {}

} // namespace acl
