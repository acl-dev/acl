#pragma once

#include "mqtt_message.hpp"

namespace acl {

class mqtt_pingresp : public mqtt_message {
public:
	mqtt_pingresp(void);
	~mqtt_pingresp(void);

protected:
	// @override
	bool to_string(string& out);

	// @override
	int update(const char*, int dlen);

	// @override
	bool is_finished(void) const {
		return true;
	}
};

} // namespace acl
