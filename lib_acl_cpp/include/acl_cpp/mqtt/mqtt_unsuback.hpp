#pragma once

#include "mqtt_ack.hpp"

namespace acl {

class mqtt_unsuback : public mqtt_ack {
public:
	mqtt_unsuback(void);
	~mqtt_unsuback(void);
};

} // namespace acl
