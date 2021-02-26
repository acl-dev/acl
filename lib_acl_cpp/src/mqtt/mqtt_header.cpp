#include "acl_stdafx.hpp"
#include "acl_cpp/mqtt/mqtt_header.hpp"

namespace acl {

static const struct mqtt_constrain __constrains[] = {
	{ MQTT_RESERVED_MIN,	0x0,	MQTT_NONE,	MQTT_NONE,	"RESERVED"	},
	{ MQTT_CONNECT,		0x0,	MQTT_NONE,	MQTT_NONE,	"CONNECT"	},
	{ MQTT_CONNACK,		0x0,	MQTT_NONE,	MQTT_NONE,	"CONNACK"	},
	{ MQTT_PUBLISH,		0xf,	MQTT_MAYBE,	MQTT_MAYBE,	"PUBLISH"	},
	{ MQTT_PUBACK,		0x0,	MQTT_NEED,	MQTT_NONE,	"PUBACK"	},
	{ MQTT_PUBREC,		0x0,	MQTT_NEED,	MQTT_NONE,	"PUBREC"	},
	{ MQTT_PUBREL,		0x2,	MQTT_NEED,	MQTT_NONE,	"PUBREL"	},
	{ MQTT_PUBCOMP,		0x0,	MQTT_NEED,	MQTT_NONE,	"PUBCOMP"	},
	{ MQTT_SUBSCRIBE,	0x2,	MQTT_NEED,	MQTT_NEED,	"SUBSCRIBE"	},
	{ MQTT_SUBACK,		0x0,	MQTT_NEED,	MQTT_NEED,	"SUBACK"	},
	{ MQTT_UNSUBSCRIBE,	0x2,	MQTT_NEED,	MQTT_NEED,	"UNSUBSCRIBE"	},
	{ MQTT_UNSUBACK,	0x0,	MQTT_NEED,	MQTT_NONE,	"UNSUBACK"	},
	{ MQTT_PINGREQ,		0x0,	MQTT_NONE,	MQTT_NONE,	"PINGREQ"	},
	{ MQTT_PINGRESP,	0x0,	MQTT_NONE,	MQTT_NONE,	"PINGRESP"	},
	{ MQTT_DISCONNECT,	0x0,	MQTT_NONE,	MQTT_NONE,	"DISCONNECT"	},
	{ MQTT_RESERVED_MAX,	0x0,	MQTT_NONE,	MQTT_NONE,	"RESERVED"	},
};

const char* mqtt_type_desc(mqtt_type_t type) {
	if (type <= MQTT_RESERVED_MIN || type >= MQTT_RESERVED_MAX) {
		return "unknown";
	}
	return __constrains[type].desc;
}

mqtt_message::mqtt_message(mqtt_type_t type)
: type_(type)
{
	assert(type > MQTT_CONNACK && type < MQTT_RESERVED_MAX);
	if (__constrains[type].id == MQTT_NEED) {
		length_ = 2;
	}
}

mqtt_message::~mqtt_message(void) {}

void mqtt_message::set_payload_length(unsigned len) {
	if (len == 0) {
		return;
	}

	if (__constrains[type_].payload != MQTT_NEED) {
		logger_error("%s: needn't payload", mqtt_type_desc(type_));
		return;
	}
}

bool mqtt_message::pack_header(string& out) {
	size_t len = 0;
	char header[5];

	memset(header, 0, sizeof(header));

	header[0] = (((char) type_) << 4) & 0xff;
	len++;

	if (length_ <= 127) {
		header[len++] = length_ & 0xef;
	} else if (length_ <= 16383) {
		header[len++] = ((unsigned char) (length_ >> 8) & 0xff) | 127;
		header[len++] = (unsigned char) length_ & 0xef;
	} else if (length_ <= 2097151) {
		header[len++] = ((unsigned char)(length_ >> 16) & 0xff) | 127;
		header[len++] = ((unsigned char)(length_ >> 8 ) & 0xff) | 127;
		header[len++] = (unsigned char) length_ & 0xef;
	} else if (length_ <= 268435455) {
		header[len++] = ((unsigned char)(length_ >> 24) & 0xff) | 127;
		header[len++] = ((unsigned char)(length_ >> 16) & 0xff) | 127;
		header[len++] = ((unsigned char)(length_ >> 8 ) & 0xff) | 127;
		header[len++] = (unsigned char) length_ & 0xef;
	} else {
		logger_error("invalid length_=%u", length_);
		return false;
	}

	out.append(header, len);
	return true;
}

void mqtt_message::pack_add(unsigned char ch, string& out) {
	out += (unsigned char) ch;
}

void mqtt_message::pack_add(unsigned short n, string& out) {
	out += (unsigned char) (n >> 8) & 0xff;
	out += (unsigned char) (n & 0xff);
}

void mqtt_message::pack_add(const string& s, string& out) {
	unsigned short n = (unsigned short) s.size();
	out += (unsigned char) (n >> 8) & 0xff;
	out += (unsigned char) (n & 0xff);
	if (n > 0) {
		out += s;
	}
}

mqtt_connect::mqtt_connect(void)
: mqtt_message(MQTT_CONNACK)
, flags_(0)
, keep_alive_(300)
{
}

mqtt_connect::~mqtt_connect(void) {}

void mqtt_connect::set_keep_alive(unsigned short keep_alive) {
	keep_alive_ = keep_alive;
}

void mqtt_connect::set_username(const char* name) {
	if (name && *name) {
		username_ = name;
		flags_ |= 1 << 7;
	}
}

void mqtt_connect::set_passwd(const char* passwd) {
	if (passwd && *passwd) {
		passwd_ = passwd;
		flags_ |= 1 << 6;
	}
}

void mqtt_connect::set_qos(mqtt_qos_t qos) {
	switch (qos) {
	case MQTT_QOS0:
		flags_ &= ~(1 << 4 | 1 << 3);
		break;
	case MQTT_QOS1:
		flags_ &= ~(1 << 4);
		flags_ |= 1 << 3;
		break;
	case MQTT_QOS2:
		flags_ &= ~(1 << 3);
		flags_ |= 1 << 4;
		break;
	default:
		logger_error("invalid qos: %d", (int) qos);
	}
}

void mqtt_connect::set_will_topic(const char* topic) {
	if (topic && *topic) {
		will_topic_ = topic;
	}
}

void mqtt_connect::set_will_msg(const char* msg) {
	if (msg && *msg) {
		will_msg_ = msg;
	}
}

bool mqtt_connect::to_string(string& out) {
	bool old_mode = out.get_bin();
	out.set_bin(true);

	unsigned len = 10;
	len += 2;
	len += (unsigned) cid_.size();
	if (!username_.empty()) {
		len += 2;
		len += (unsigned) username_.size();
	}
	if (!passwd_.empty()) {
		len += 2;
		len += (unsigned) passwd_.size();
	}
	if (!will_topic_.empty() && !will_msg_.empty()) {
		flags_ |= 1 << 2;

		len += 2;
		len += (unsigned) will_topic_.size();

		len += 2;
		len += (unsigned) will_msg_.size();
	} else {
		will_topic_.clear();
		will_msg_.clear();
	}

	this->set_payload_length(len);
	if (!this->pack_header(out)) {
		out.set_bin(old_mode);
		return false;
	}

	this->pack_add((unsigned char) 0, out);
	this->pack_add((unsigned char) 0x04, out);
	this->pack_add((unsigned char) 'M', out);
	this->pack_add((unsigned char) 'Q', out);
	this->pack_add((unsigned char) 'T', out);
	this->pack_add((unsigned char) 'T', out);
	this->pack_add((unsigned char) 0x04, out);
	this->pack_add((unsigned char) flags_, out);
	this->pack_add((unsigned short) keep_alive_, out);

	this->pack_add(cid_, out);

	if (!username_.empty()) {
		this->pack_add(username_, out);
	}

	if (!passwd_.empty()) {
		this->pack_add(passwd_, out);
	}

	if (!will_topic_.empty() && !will_msg_.empty()) {
		this->pack_add(will_topic_, out);
		this->pack_add(will_msg_, out);
	}

	out.set_bin(old_mode);
	return true;
}

} //namespace acl
