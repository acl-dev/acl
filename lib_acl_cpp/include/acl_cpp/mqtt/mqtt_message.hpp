#pragma once

#include "../stdlib/string.hpp"

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

enum {
	MQTT_STAT_STR_LEN	= 0,
	MQTT_STAT_STR_VAL	= 1,
	MQTT_STAT_HDR_VAR	= 2,
	MQTT_STAT_HDR_END	= 3,
};

typedef enum {
	MQTT_NONE,
	MQTT_NEED,
	MQTT_MAYBE,
} mqtt_option_t;

struct mqtt_constrain {
	mqtt_type_t type;
	unsigned char flags:4;
	mqtt_option_t id;
	mqtt_option_t payload;
	const char* desc;
};

struct mqtt_header {
	mqtt_type_t   type;
	unsigned char flags:4;
	unsigned int  length;
};

const char* mqtt_type_desc(mqtt_type_t type);

class mqtt_message {
public:
	mqtt_message(mqtt_type_t type);
	virtual ~mqtt_message(void);

	int header_update(const char* data, unsigned dlen);

	bool header_finish(void) const {
		return header_finished_;
	}

	mqtt_type_t get_type(void) const {
		return type_;
	}

	unsigned get_data_length(void) const {
		return dlen_;
	}

protected:
	unsigned status_;
	mqtt_type_t type_;

	void set_data_length(unsigned len);

	bool pack_header(string& out);
	void pack_add(unsigned char ch, string& out);
	void pack_add(unsigned short n, string& out);
	void pack_add(const string& s, string& out);

	bool unpack_char(const char* data, size_t len, unsigned char& out);
	bool unpack_short(const char* data, size_t len, unsigned short& out);
	void unpack_string_await(string& buf, int next);

private:
	bool header_finished_;
	unsigned char hflags_:4;

	char hbuf_[4];
	unsigned hlen_;
	unsigned dlen_;

	int status_next_;
	string* buff_;
	unsigned short vlen_;

public:
	int unpack_string_len(const char* data, unsigned dlen);
	int unpack_string_val(const char* data, unsigned dlen);

	int unpack_header_type(const char* data, unsigned dlen);
	int unpack_header_len(const char* data, unsigned dlen);
};

typedef enum {
	MQTT_QOS0	= 0x00,
	MQTT_QOS1	= 0x01,
	MQTT_QOS2	= 0x02,
} mqtt_qos_t;

} // namespace acl