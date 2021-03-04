#pragma once

#include "mqtt_message.hpp"

namespace acl {

class mqtt_pingreq : public mqtt_message {
public:
	mqtt_pingreq(void);
	~mqtt_pingreq(void);

	bool to_string(string& out);
};

} // namespace acl
