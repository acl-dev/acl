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
	MQTT_STAT_HDR_TYPE	= 0,
	MQTT_STAT_HDR_LEN	= 1,
	MQTT_STAT_HDR_VAR	= 2,
	MQTT_STAT_STR_LEN	= 4,
	MQTT_STAT_STR_VAL	= 5,

	MQTT_STAT_CID		= 6,
	MQTT_STAT_USERNAME	= 7,
	MQTT_STAT_PASSWD	= 8,
	MQTT_STAT_WILL_TOPIC	= 9,
	MQTT_STAT_WILL_MSG	= 9,
} mqtt_status_t;

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
	mqtt_status_t status_;

	mqtt_type_t type_;

	void set_data_length(unsigned len);
	bool pack_header(string& out);
	void pack_add(unsigned char ch, string& out);
	void pack_add(unsigned short n, string& out);
	void pack_add(const string& s, string& out);

	bool fix_header_finish(void) const {
		return status_ != MQTT_STAT_HDR_TYPE
			&& status_ != MQTT_STAT_HDR_LEN;
	}

	unsigned get_data_length(void) const {
		return dlen_;
	}

	int unpack_header(const char* data, unsigned dlen);

	void unpack_string_await(string& buf, mqtt_status_t next);

	bool unpack_char(const char* data, size_t len, unsigned char& out);
	bool unpack_short(const char* data, size_t len, unsigned short& out);

private:
	unsigned char hflags_:4;

	char hbuf_[4];
	unsigned hlen_;
	unsigned dlen_;

	mqtt_status_t next_;
	string* buff_;
	unsigned short vlen_;

public:
	int unpack_string_len(const char* data, unsigned dlen);
	int unpack_string_val(const char* data, unsigned dlen);

	int unpack_header_type(const char* data, unsigned dlen);
	int unpack_header_len(const char* data, unsigned dlen);
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

	int update(const char* data, unsigned dlen);

	bool is_finished(void) const {
		return finished_;
	}

	unsigned short get_keep_alive(void) const {
		return keep_alive_;
	}

	const char* get_username(void) const {
		return username_.empty() ? NULL : username_.c_str();
	}

	const char* get_passwd(void) const {
		return passwd_.empty() ? NULL : passwd_.c_str();
	}

	mqtt_qos_t get_qos(void) const {
		return qos_;
	}

	const char* get_will_topic(void) const {
		return will_topic_.empty() ? NULL : will_topic_.c_str();
	}

	const char* get_will_msg(void) const {
		return will_msg_.empty() ? NULL : will_msg_.c_str();
	}

private:
	char hbuf_[10];
	unsigned hlen_;

	bool finished_;

	mqtt_qos_t qos_;
	unsigned char conn_flags_;
	unsigned short keep_alive_;

	string cid_;
	string username_;
	string passwd_;
	string will_topic_;
	string will_msg_;

public:
	int unpack_header_var(const char* data, unsigned dlen);
	int unpack_cid(const char* data, unsigned dlen);
	int unpack_username(const char* data, unsigned dlen);
	int unpack_passwd(const char* data, unsigned dlen);
	int unpack_will_topic(const char* data, unsigned dlen);
	int unpack_will_msg(const char* data, unsigned dlen);
};

class mqtt_connack : public mqtt_message {
public:
	mqtt_connack(void);
	~mqtt_connack(void);
};

} // namespace acl
