#pragma once

#include "mqtt_ack.hpp"

namespace acl {

class ACL_CPP_API mqtt_pubcomp : public mqtt_ack {
public:
	mqtt_pubcomp(void);
	~mqtt_pubcomp(void);
};

} // namespace acl
