#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_pingresp.hpp"
#endif

namespace acl {

mqtt_pingresp::mqtt_pingresp(void)
: mqtt_message(MQTT_PINGRESP)
{
}

mqtt_pingresp::mqtt_pingresp(const mqtt_header& header)
: mqtt_message(header)
{
}

mqtt_pingresp::~mqtt_pingresp(void) {}

bool mqtt_pingresp::to_string(string& out) {
	mqtt_header& header = this->get_header();
	header.set_remaing_length(0);

	if (!header.build_header(out)) {
		return false;
	}
	return true;
}

} // namespace acl
