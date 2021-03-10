#pragma once

#include "mqtt_message.hpp"

namespace acl {

class ACL_CPP_API mqtt_pingreq : public mqtt_message {
public:
	mqtt_pingreq(void);
	mqtt_pingreq(const mqtt_header& header);
	~mqtt_pingreq(void);

protected:
	// @override
	bool to_string(string& out);

	// @override
	int update(const char*, int dlen) {
		return dlen;
	}

	// @override
	bool finished(void) const {
		return true;
	}
};

} // namespace acl
