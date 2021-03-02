#pragma once

#include "mqtt_puback.hpp"

namespace acl {

class mqtt_pubrel : public mqtt_puback {
public:
	mqtt_pubrel(void);
	~mqtt_pubrel(void);
};

} // namespace acl
