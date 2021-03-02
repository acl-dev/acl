#pragma once

#include "mqtt_puback.hpp"

namespace acl {

class mqtt_pubrec : public mqtt_puback {
public:
	mqtt_pubrec(void);
	~mqtt_pubrec(void);

protected:
	// @override from mqtt_message
	unsigned char get_header_flags(void) const {
		return 0x02;
	}
};

} // namespace acl
