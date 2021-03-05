#pragma once

#include "mqtt_message.hpp"

namespace acl {

class mqtt_disconnect : public mqtt_message {
public:
	mqtt_disconnect(void);
	~mqtt_disconnect(void);

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
