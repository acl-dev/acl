#pragma once

#include "../acl_cpp_define.hpp"

namespace acl {

/**
 * all the mqtt types were defined below.
 */
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

/**
 * all the qos type of mqtt
 */
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

/**
 * get description of the specified mqtt type.
 * @param type {mqtt_type_t}
 * @return {const char*}
 */
const char* mqtt_type_desc(mqtt_type_t type);

/**
 * get description of the specified mqtt qos.
 * @param qos {mqtt_qos_t}
 * @return {const char*}
 */
const char* mqtt_qos_desc(mqtt_qos_t qos);

class string;

/**
 * mqtt message header class, used for building or parsing mqtt header data.
 */
class ACL_CPP_API mqtt_header {
public:
	/**
	 * mqtt header constructor, usually for building mqtt message.
	 * @param type {mqtt_type_t}
	 */
	mqtt_header(mqtt_type_t type);

	/**
	 * mqtt header constructor, usually for parsing mqtt message.
	 * @param header {const mqtt_header&} will be copied internal.
	 */
	mqtt_header(const mqtt_header& header);

	virtual ~mqtt_header(void);

public:
	/**
	 * build mqtt header data after initializing the header object
	 * by calling the setting methods below like set_xxx.
	 * @param out {string&} store mqtt header data.
	 * @return {bool} return true if build header successfully.
	 */
	bool build_header(string& out);

	/**
	 * parsing mqtt header data in streaming mode.
	 * @param data {const char*} the mqtt header data, not NULL.
	 * @param dlen {int} the length of data, must > 0
	 * @return {int} return the length of the left data not consumed:
	 *  > 0: the header has completed and the length of left data can
	 *       be used as mqtt body or next mqtt message;
	 *    0: the data input has been consumed, you can call finished() to
	 *       check if the mqtt header has completed.
	 *   -1: some error happened when parsing the input data.
	 */
	int update(const char* data, int dlen);

	/**
	 * check if the mqtt header has completed.
	 * @return {bool}
	 */
	bool finished(void) const {
		return finished_;
	}

	/**
	 * reset the status of the mqtt header object for reusing the object.
	 */
	void reset(void);

public:
	/**
	 * set the mqtt message type in mqtt header
	 * @param type {mqtt_type_t}
	 * @return {mqtt_header&}
	 */
	mqtt_header& set_type(mqtt_type_t type);

	/**
	 * set the mqtt header flags.
	 * @param flags {char}
	 * @return {mqtt_header&}
	 */
	mqtt_header& set_header_flags(char flags);

	/**
	 * set the length of the mqtt message body.
	 * @param len {unsigned}
	 * @return {mqtt_header&}
	 */
	mqtt_header& set_remaing_length(unsigned len);

	/**
	 * set the qos of the mqtt message.
	 * @param qos {mqtt_qos_t}
	 * @return {mqtt_header&}
	 */
	mqtt_header& set_qos(mqtt_qos_t qos);

	/**
	 * set if the mqtt message be sent duplicated.
	 * @param yes {bool}
	 * @return {mqtt_header&}
	 */
	mqtt_header& set_dup(bool yes);

	/**
	 * set the remain flag in mqtt header.
	 * @param yes {bool}
	 * @return {mqtt_header&}
	 */
	mqtt_header& set_remain(bool yes);

	/**
	 * get the mqtt message type.
	 * @return {mqtt_type_t}
	 */
	mqtt_type_t get_type(void) const {
		return type_;
	}

	/**
	 * get the mqtt header flags.
	 * @return {unsigned char}
	 */
	unsigned char get_header_flags(void) const {
		return hflags_;
	}

	/**
	 * get the length of the mqtt message body.
	 * @return {unsigned}
	 */
	unsigned get_remaining_length(void) const {
		return dlen_;
	}

	/**
	 * get the mqtt message's qos.
	 * @return {mqtt_qos_t}
	 */
	mqtt_qos_t get_qos(void) const;

	/**
	 * check if the duplicated flag has been set in header.
	 * @return {bool}
	 */
	bool is_dup(void) const;

	/**
	 * check if the remain flag has been set in header.
	 * @return  {bool}
	 */
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
	// parsing mqtt header in streaming mode, return the length of left
	// data that not consumed.

	int update_header_type(const char* data, int dlen);
	int update_header_len(const char* data, int dlen);
};

} // namespace acl
