#pragma once

#include "mqtt_ack.hpp"

namespace acl {

class mqtt_puback : public mqtt_ack {
public:
	mqtt_puback(void);
	~mqtt_puback(void);
};

} // namespace acl
