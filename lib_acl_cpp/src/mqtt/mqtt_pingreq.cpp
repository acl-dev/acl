#include "acl_stdafx.hpp"
#include "acl_cpp/mqtt/mqtt_pingreq.hpp"

namespace acl {

mqtt_pingreq::mqtt_pingreq()
: mqtt_message(MQTT_PINGREQ)
{
}

mqtt_pingreq::~mqtt_pingreq(void) {}

bool mqtt_pingreq::to_string(string& out) {
	bool old_mode = out.get_bin();

	this->set_data_length(0);

	if (!this->pack_header(out)) {
		out.set_bin(old_mode);
		return false;
	}

	out.set_bin(old_mode);
	return true;
}

} // namespace acl
