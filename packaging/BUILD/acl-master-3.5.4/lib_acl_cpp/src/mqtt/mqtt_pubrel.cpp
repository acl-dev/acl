#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_pubrel.hpp"
#endif

namespace acl {

mqtt_pubrel::mqtt_pubrel(void)
: mqtt_ack(MQTT_PUBREL)
{
	this->get_header().set_header_flags(0x02);
}

mqtt_pubrel::mqtt_pubrel(const mqtt_header& header)
: mqtt_ack(header)
{
	this->get_header().set_header_flags(0x02);
}

mqtt_pubrel::~mqtt_pubrel(void) {}

} // namespace acl
