#pragma once

#include "mqtt_ack.hpp"

namespace acl {

class ACL_CPP_API mqtt_pubrel : public mqtt_ack {
public:
	mqtt_pubrel(void);
	~mqtt_pubrel(void);
};

} // namespace acl
