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

protected:
	mqtt_type_t type_;
	unsigned length_;

	void set_payload_length(unsigned len);
	bool pack_header(string& out);
	void pack_add(unsigned char ch, string& out);
	void pack_add(unsigned short n, string& out);
	void pack_add(const string& s, string& out);
};

typedef enum {
	CONNECT_ACCEPTED	= 0x00,
	CONNECT_INVALID_VERSION	= 0x01,
	CONNECT_INVALID_CID	= 0x02,
	CONNECT_NOT_AVAIL	= 0x03,
	CONNECT_LOGIN_FAILED	= 0x04,
	CONNECT_NO_AUTHORITY	= 0x05,
} mqtt_conn_status_t;

typedef enum {
	MQTT_QOS0	= 0x00,
	MQTT_QOS1	= 0x01,
	MQTT_QOS2	= 0x02,
} mqtt_qos_t;

class mqtt_connect : public mqtt_message {
public:
	mqtt_connect(void);
	~mqtt_connect(void);

	void set_keep_alive(unsigned short keep_alive);
	void set_username(const char* name);
	void set_passwd(const char* passwd);
	void set_qos(mqtt_qos_t qos);
	void set_will_topic(const char* topic);
	void set_will_msg(const char* msg);

	bool to_string(string& out);

private:
	unsigned char flags_;
	unsigned short keep_alive_;

	string cid_;
	string username_;
	string passwd_;
	string will_topic_;
	string will_msg_;
};

} // namespace acl
