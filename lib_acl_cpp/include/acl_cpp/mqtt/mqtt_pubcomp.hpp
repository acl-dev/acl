#pragma once

#include "mqtt_ack.hpp"

namespace acl {

class mqtt_pubcomp : public mqtt_ack {
public:
	mqtt_pubcomp(void);
	~mqtt_pubcomp(void);
};

} // namespace acl
