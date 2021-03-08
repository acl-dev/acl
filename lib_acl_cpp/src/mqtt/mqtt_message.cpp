#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_message.hpp"
#endif

namespace acl {

mqtt_message::mqtt_message(mqtt_type_t type)
: header_(type)
{
	assert(type >= MQTT_RESERVED_MIN && type < MQTT_RESERVED_MAX);
}

mqtt_message::mqtt_message(const mqtt_header& header)
: header_(header)
{
}

mqtt_message::~mqtt_message(void) {}

void mqtt_message::pack_add(unsigned char ch, string& out) {
	out.append(&ch, 1);
}

//#define MOSQ_MSB(x) (unsigned char)((x & 0xff00) >> 8)
//#define MOSQ_LSB(x) (unsigned char)(x & 0x00ff)

#define	MOSQ_MSB(x) (unsigned char) ((x >> 8) & 0xff)
#define MOSQ_LSB(x) (unsigned char) (x & 0xff)

void mqtt_message::pack_add(unsigned short n, string& out) {
	unsigned char ch = MOSQ_MSB(n);
	out.append(&ch, 1);

	ch = MOSQ_LSB(n);
	out.append(&ch, 1);
}

void mqtt_message::pack_add(const string& s, string& out) {
	unsigned short n = (unsigned short) s.size();

	unsigned char ch = MOSQ_MSB(n);
	out.append(&ch, 1);

	ch = MOSQ_LSB(n);
	out.append(&ch, 1);

	if (n > 0) {
		out += s;
	}
}

bool mqtt_message::unpack_short(const char* in, size_t len, unsigned short& out) {
	if (len < 2) {
		logger_error("too short: %ld", (long) len);
		return false;
	}

	out = (unsigned short) (((unsigned short) (in[0] & 0xff) << 8)
			| (unsigned short) (in[1] & 0xff));
	return true;
}

} //namespace acl
