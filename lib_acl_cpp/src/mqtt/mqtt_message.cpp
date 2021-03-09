#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_connect.hpp"
#include "acl_cpp/mqtt/mqtt_connack.hpp"
#include "acl_cpp/mqtt/mqtt_publish.hpp"
#include "acl_cpp/mqtt/mqtt_puback.hpp"
#include "acl_cpp/mqtt/mqtt_pubrec.hpp"
#include "acl_cpp/mqtt/mqtt_pubrel.hpp"
#include "acl_cpp/mqtt/mqtt_pubcomp.hpp"
#include "acl_cpp/mqtt/mqtt_subscribe.hpp"
#include "acl_cpp/mqtt/mqtt_suback.hpp"
#include "acl_cpp/mqtt/mqtt_unsubscribe.hpp"
#include "acl_cpp/mqtt/mqtt_unsuback.hpp"
#include "acl_cpp/mqtt/mqtt_pingreq.hpp"
#include "acl_cpp/mqtt/mqtt_pingresp.hpp"
#include "acl_cpp/mqtt/mqtt_disconnect.hpp"

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

mqtt_message* mqtt_message::create_message(const mqtt_header& header) {
	mqtt_type_t type = header.get_type();
	mqtt_message* message;

	switch (type) {
	case MQTT_CONNECT:
		message = NEW mqtt_connect(header);
		break;
	case MQTT_CONNACK:
		message = NEW mqtt_connack(header);
		break;
	case MQTT_PUBLISH:
		message = NEW mqtt_publish(header);
		break;
	case MQTT_PUBACK:
		message = NEW mqtt_puback(header);
		break;
	case MQTT_PUBREC:
		message = NEW mqtt_pubrec(header);
		break;
	case MQTT_PUBREL:
		message = NEW mqtt_pubrel(header);
		break;
	case MQTT_PUBCOMP:
		message = NEW mqtt_pubcomp(header);
		break;
	case MQTT_SUBSCRIBE:
		message = NEW mqtt_subscribe(header);
		break;
	case MQTT_SUBACK:
		message = NEW mqtt_suback(header);
		break;
	case MQTT_UNSUBSCRIBE:
		message = NEW mqtt_unsubscribe(header);
		break;
	case MQTT_UNSUBACK:
		message = NEW mqtt_unsuback(header);
		break;
	case MQTT_PINGREQ:
		message = NEW mqtt_pingreq(header);
		break;
	case MQTT_PINGRESP:
		message = NEW mqtt_pingresp(header);
		break;
	case MQTT_DISCONNECT:
		message = NEW mqtt_disconnect(header);
		break;
	default:
		logger_error("unknown mqtt type=%d", (int) type);
		message = NULL;
		break;
	}
	return message;
}

} //namespace acl
