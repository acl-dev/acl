#pragma once

#include "mqtt_message.hpp"

namespace acl {

class mqtt_disconnect : public mqtt_message {
public:
	mqtt_disconnect(void);
	~mqtt_disconnect(void);

	bool to_string(string& out);
};

} // namespace acl
