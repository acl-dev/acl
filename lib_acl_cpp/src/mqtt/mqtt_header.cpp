#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_header.hpp"
#endif

namespace acl {

static const struct mqtt_constrain __constrains[] = {
	{ MQTT_RESERVED_MIN,	0x0,	MQTT_NONE,	MQTT_NONE,	"RESERVED"	},
	{ MQTT_CONNECT,		0x0,	MQTT_NONE,	MQTT_NEED,	"CONNECT"	},
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

const char* mqtt_qos_desc(mqtt_qos_t qos) {
	switch (qos) {
	case MQTT_QOS0:
		return "QOS0";
	case MQTT_QOS1:
		return "QOS1";
	case MQTT_QOS2:
		return "QOS2";
	default:
		return "unknown";
	}
}

enum {
	MQTT_STAT_HDR_TYPE,
	MQTT_STAT_HDR_LEN,
};

mqtt_header::mqtt_header(mqtt_type_t type)
: status_(MQTT_STAT_HDR_TYPE)
, finished_(false)
, type_(type)
, hflags_(0)
, dlen_(0)
, hlen_(0)
{
	assert(type >= MQTT_RESERVED_MIN && type < MQTT_RESERVED_MAX);
	if (__constrains[type].id == MQTT_NEED) {
	}
}

mqtt_header::mqtt_header(const mqtt_header& header)
: status_(MQTT_STAT_HDR_TYPE)
, finished_(false)
, hlen_(0)
{
	type_   = header.get_type();
	hflags_ = header.get_header_flags();
	dlen_   = header.get_remaining_length();
}

mqtt_header::~mqtt_header(void) {}

void mqtt_header::reset(void) {
	status_ = MQTT_STAT_HDR_TYPE;
	finished_ = false;
	type_   = MQTT_RESERVED_MIN;
	hflags_ = 0;
	dlen_   = 0;
	hlen_   = 0;
}

mqtt_header& mqtt_header::set_type(mqtt_type_t type) {
	if (type > MQTT_RESERVED_MIN && type < MQTT_RESERVED_MAX) {
		type_ = type;
	} else {
		logger_error("invalid type=%d", (int) type);
	}
	return *this;
}

mqtt_header& mqtt_header::set_header_flags(char flags) {
	hflags_ = flags & 0x0f;
	return *this;
}

mqtt_header& mqtt_header::set_remaing_length(unsigned len) {
	dlen_ = len;
	return *this;
}

mqtt_header& mqtt_header::set_qos(mqtt_qos_t qos) {
	hflags_ &= ~(1 << 2 | 1 << 1);
	switch (qos) {
	case MQTT_QOS1:
		hflags_ |= 1 << 1;
		break;
	case MQTT_QOS2:
		hflags_ |= 1 << 2;
		break;
	case MQTT_QOS0:
	default:
		break;
	}
	return *this;
}

mqtt_header& mqtt_header::set_dup(bool yes) {
	if (yes) {
		hflags_ |= 1 << 3;
	} else {
		hflags_ &= ~(1 << 3);
	}
	return *this;
}

mqtt_header& mqtt_header::set_remain(bool yes) {
	if (yes) {
		hflags_ |= 0x01;
	} else {
		hflags_ &= ~0x01;
	}
	return *this;
}

bool mqtt_header::build_header(string& out) {
	size_t len = 0;
	char header[5];

	memset(header, 0, sizeof(header));

	unsigned char flags = this->get_header_flags();
	hflags_ |= (flags & 0x0f);

	header[len] = ((((char) type_) << 4) & 0xff) | hflags_;
	len++;

	if (dlen_ < 128) {
		header[len++] = dlen_ & 0x7f;
	} else if (dlen_ < 16384) {
		header[len++] = ((unsigned char) dlen_ & 0x7f) | 0x80;
		header[len++] = ((unsigned char) (dlen_ >> 7) & 0x7f);
	} else if (dlen_ < 2097152) {
		header[len++] = ((unsigned char) dlen_ & 0x7f) | 0x80;
		header[len++] = ((unsigned char) (dlen_ >> 7 ) & 0x7f) | 0x80;
		header[len++] = ((unsigned char) (dlen_ >> 14) & 0x7f);
	} else if (dlen_ < 268435456) {
		header[len++] = ((unsigned char) dlen_ & 0x7f) | 0x80;
		header[len++] = ((unsigned char) (dlen_ >> 7 ) & 0x7f) | 0x80;
		header[len++] = ((unsigned char) (dlen_ >> 14) & 0x7f) | 0x80;
		header[len++] = ((unsigned char) (dlen_ >> 21) & 0x7f);
	} else {
		logger_error("invalid dlen_=%u", dlen_);
		return false;
	}

	out.append(header, len);
	return true;
}

mqtt_qos_t mqtt_header::get_qos(void) const {
	switch ((hflags_ >> 1) & 0x0f) {
	case MQTT_QOS1:
		return MQTT_QOS1;
	case MQTT_QOS2:
		return MQTT_QOS2;
	case 0:
	default:
		return MQTT_QOS0;
	}
}

bool mqtt_header::is_dup(void) const {
	return (hflags_ & 0x08) ? true : false;
}

bool mqtt_header::is_remain(void) const {
	return (hflags_ & 0x01) ? true : false;
}

static struct {
	int status;
	int (mqtt_header::*handler)(const char*, int);
} handlers[] = {
	{ MQTT_STAT_HDR_TYPE,	&mqtt_header::update_header_type	},
	{ MQTT_STAT_HDR_LEN,	&mqtt_header::update_header_len		},
};

int mqtt_header::update(const char* data, int dlen) {
	if (data == NULL || dlen <= 0) {
		logger_error("invalid input");
		return -1;
	}

	while (dlen > 0 && !finished_) {
		int ret = (this->*handlers[status_].handler)(data, dlen);
		if (ret < 0) {
			return -1;
		}
		data += dlen - ret;
		dlen  = ret;
	}

	return dlen;
}

int mqtt_header::update_header_type(const char* data, int dlen) {
	assert(data && dlen > 0);

	char ch = *data++;
	dlen--;

	char type = (ch &0xff) >> 4;
	if (type <= MQTT_RESERVED_MIN || type >= MQTT_RESERVED_MAX) {
		logger_error("invalid type=%d", type);
		return -1;
	}

	type_   = (mqtt_type_t) type;
	hflags_ = ch & 0x0f;
	status_ = MQTT_STAT_HDR_LEN;
	hlen_   = 0;
	return dlen;
}

int mqtt_header::update_header_len(const char* data, int dlen) {
	assert(data && dlen > 0);

	if (hlen_ >= sizeof(hbuf_)) {
		logger_error("invalid header len");
		return -1;
	}

	for (; hlen_ < sizeof(hbuf_) && dlen > 0;) {
		hbuf_[hlen_] = *data++;
		dlen--;

		if ((hbuf_[hlen_++] & 0x80) == 0) {
			break;
		}
	}

	assert(hlen_ > 0 && hlen_ <= sizeof(hbuf_));

	if ((hbuf_[hlen_ - 1] & 0x80) != 0) {
		if (hlen_ == 4) {
			logger_error("invalid header len");
			return -1;
		}
		assert(dlen == 0);
		return dlen;
	}

	dlen_ = 0;
	for (unsigned i = 0; i < hlen_; i++) {
		dlen_ |= (unsigned) ((hbuf_[i] & 0x7f) << (7 * i));
	}

	finished_ = true;
	return dlen;
}

} //namespace acl
