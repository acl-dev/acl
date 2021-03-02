#pragma once

#include "mqtt_puback.hpp"

namespace acl {

class mqtt_pubcomp : public mqtt_puback {
public:
	mqtt_pubcomp(void);
	~mqtt_pubcomp(void);
};

} // namespace acl
