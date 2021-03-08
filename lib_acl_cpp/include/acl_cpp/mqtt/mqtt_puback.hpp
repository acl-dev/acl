#pragma once

#include "mqtt_ack.hpp"

namespace acl {

class ACL_CPP_API mqtt_puback : public mqtt_ack {
public:
	mqtt_puback(void);
	mqtt_puback(const mqtt_header& header);
	~mqtt_puback(void);
};

} // namespace acl
