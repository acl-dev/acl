#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_pingreq.hpp"
#endif

namespace acl {

mqtt_pingreq::mqtt_pingreq(void)
: mqtt_message(MQTT_PINGREQ)
{
}

mqtt_pingreq::mqtt_pingreq(const mqtt_header& header)
: mqtt_message(header)
{
}

mqtt_pingreq::~mqtt_pingreq(void) {}

bool mqtt_pingreq::to_string(string& out) {
	mqtt_header& header = this->get_header();

	header.set_remaing_length(0);

	if (!header.build_header(out)) {
		return false;
	}
	return true;
}

} // namespace acl
