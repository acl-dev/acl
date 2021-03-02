#pragma once

#include "mqtt_puback.hpp"

namespace acl {

class mqtt_pubrec : public mqtt_puback {
public:
	mqtt_pubrec(void);
	~mqtt_pubrec(void);
};

} // namespace acl
