#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_pingreq.hpp"
#endif

namespace acl {

mqtt_pingreq::mqtt_pingreq()
: mqtt_message(MQTT_PINGREQ)
{
}

mqtt_pingreq::~mqtt_pingreq(void) {}

bool mqtt_pingreq::to_string(string& out) {
	bool old_mode = out.get_bin();
	out.set_bin(true);

	this->set_data_length(0);

	if (!this->pack_header(out)) {
		out.set_bin(old_mode);
		return false;
	}

	out.set_bin(old_mode);
	return true;
}

} // namespace acl
