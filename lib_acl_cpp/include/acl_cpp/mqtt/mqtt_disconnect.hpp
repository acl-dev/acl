#pragma once

#include "mqtt_message.hpp"

namespace acl {

class ACL_CPP_API mqtt_disconnect : public mqtt_message {
public:
	mqtt_disconnect(void);
	mqtt_disconnect(const mqtt_header& header);
	~mqtt_disconnect(void);

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
