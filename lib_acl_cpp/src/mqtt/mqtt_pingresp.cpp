#include "acl_stdafx.hpp"
#include "acl_cpp/mqtt/mqtt_pingresp.hpp"

namespace acl {

mqtt_pingresp::mqtt_pingresp()
: mqtt_message(MQTT_PINGRESP)
{
}

mqtt_pingresp::~mqtt_pingresp(void) {}

bool mqtt_pingresp::to_string(string& out) {
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
