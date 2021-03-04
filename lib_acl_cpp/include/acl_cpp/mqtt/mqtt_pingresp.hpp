#pragma once

#include "mqtt_message.hpp"

namespace acl {

class mqtt_pingresp : public mqtt_message {
public:
	mqtt_pingresp(void);
	~mqtt_pingresp(void);

	bool to_string(string& out);
};

} // namespace acl
