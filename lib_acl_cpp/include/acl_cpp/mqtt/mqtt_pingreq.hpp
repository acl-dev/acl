#pragma once

#include "mqtt_message.hpp"

namespace acl {

class mqtt_pingreq : public mqtt_message {
public:
	mqtt_pingreq(void);
	~mqtt_pingreq(void);

protected:
	// @override
	bool to_string(string& out);

	// @override
	int update(const char*, int dlen) {
		return dlen;
	}

	// @override
	bool is_finished(void) const {
		return true;
	}
};

} // namespace acl
