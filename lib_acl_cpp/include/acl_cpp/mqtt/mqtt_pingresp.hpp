#pragma once

#include "mqtt_message.hpp"

namespace acl {

class ACL_CPP_API mqtt_pingresp : public mqtt_message {
public:
	mqtt_pingresp(void);
	mqtt_pingresp(const mqtt_header& header);
	~mqtt_pingresp(void);

protected:
	// @override
	bool to_string(string& out);

	// @override
	bool finished(void) const {
		return true;
	}
};

} // namespace acl
