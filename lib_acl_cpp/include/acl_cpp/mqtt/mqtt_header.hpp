#pragma once

#include "../acl_cpp_define.hpp"

namespace acl {

typedef enum {
	MQTT_RESERVED_MIN	= 0,
	MQTT_CONNECT		= 1,
	MQTT_CONNACK		= 2,
	MQTT_PUBLISH		= 3,
	MQTT_PUBACK		= 4,
	MQTT_PUBREC		= 5,
	MQTT_PUBREL		= 6,
	MQTT_PUBCOMP		= 7,
	MQTT_SUBSCRIBE		= 8,
	MQTT_SUBACK		= 9,
	MQTT_UNSUBSCRIBE	= 10,
	MQTT_UNSUBACK		= 11,
	MQTT_PINGREQ		= 12,
	MQTT_PINGRESP		= 13,
	MQTT_DISCONNECT		= 14,
	MQTT_RESERVED_MAX	= 15,
} mqtt_type_t;

typedef enum {
	MQTT_NONE,
	MQTT_NEED,
	MQTT_MAYBE,
} mqtt_option_t;

typedef enum {
	MQTT_QOS0	= 0x0,
	MQTT_QOS1	= 0x1,
	MQTT_QOS2	= 0x2,
} mqtt_qos_t;

struct mqtt_constrain {
	mqtt_type_t type;
	unsigned char flags:4;
	mqtt_option_t id;
	mqtt_option_t payload;
	const char* desc;
};

const char* mqtt_type_desc(mqtt_type_t type);
const char* mqtt_qos_desc(mqtt_qos_t qos);

class string;

class ACL_CPP_API mqtt_header {
public:
	mqtt_header(mqtt_type_t type);
	mqtt_header(const mqtt_header& header);
	virtual ~mqtt_header(void);

public:
	bool build_header(string& out);

	int update(const char* data, int dlen);

	bool finished(void) const {
		return finished_;
	}

	void reset(void);

public:
	mqtt_header& set_type(mqtt_type_t type);
	mqtt_header& set_header_flags(char flags);
	mqtt_header& set_remaing_length(unsigned len);

	mqtt_header& set_qos(mqtt_qos_t qos);
	mqtt_header& set_dup(bool yes);
	mqtt_header& set_remain(bool yes);

	mqtt_type_t get_type(void) const {
		return type_;
	}

	unsigned char get_header_flags(void) const {
		return hflags_;
	}

	unsigned get_remaining_length(void) const {
		return dlen_;
	}

	mqtt_qos_t get_qos(void) const;
	bool is_dup(void) const;
	bool is_remain(void) const;

private:
	unsigned status_;
	bool finished_;

	mqtt_type_t type_;
	unsigned char hflags_:4;
	unsigned dlen_;

	char hbuf_[4];
	unsigned hlen_;

public:
	int update_header_type(const char* data, int dlen);
	int update_header_len(const char* data, int dlen);
};

} // namespace acl
