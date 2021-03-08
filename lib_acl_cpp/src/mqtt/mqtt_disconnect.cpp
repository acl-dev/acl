#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_disconnect.hpp"
#endif

namespace acl {

mqtt_disconnect::mqtt_disconnect(void)
: mqtt_message(MQTT_DISCONNECT)
{
}

mqtt_disconnect::mqtt_disconnect(const mqtt_header& header)
: mqtt_message(header)
{
}

mqtt_disconnect::~mqtt_disconnect(void) {}

bool mqtt_disconnect::to_string(string& out) {
	mqtt_header& header = this->get_header();
	header.set_remaing_length(0);

	if (!header.build_header(out)) {
		return false;
	}
	return true;
}

} // namespace acl
