#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_message.hpp"
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

enum {
	MQTT_STAT_HDR_TYPE,
	MQTT_STAT_HDR_LEN,
};

mqtt_message::mqtt_message(mqtt_type_t type)
: status_(MQTT_STAT_HDR_TYPE)
, type_(type)
, header_finished_(false)
, hflags_(0)
, hlen_(0)
, dlen_(0)
{
	assert(type >= MQTT_RESERVED_MIN && type < MQTT_RESERVED_MAX);
	if (__constrains[type].id == MQTT_NEED) {
	}
}

mqtt_message::~mqtt_message(void) {}

void mqtt_message::set_data_length(unsigned len) {
	dlen_ = len;
}

bool mqtt_message::pack_header(string& out) {
	size_t len = 0;
	char header[5];

	memset(header, 0, sizeof(header));

	unsigned char flags = this->get_header_flags();
	hflags_ |= (flags & 0x0f);

	header[len] = ((((char) type_) << 4) & 0xff) | hflags_;
	len++;

	if (dlen_ <= 127) {
		header[len++] = dlen_ & 0x7f;
	} else if (dlen_ <= 16383) {
		header[len++] = ((unsigned char) (dlen_ >> 8) & 0x7f) | 0x80;
		header[len++] = (unsigned char) dlen_ & 0x7f;
	} else if (dlen_ <= 2097151) {
		header[len++] = ((unsigned char)(dlen_ >> 16) & 0x7f) | 0x80;
		header[len++] = ((unsigned char)(dlen_ >> 8 ) & 0x7f) | 0x80;
		header[len++] = (unsigned char)  dlen_ & 0x7f;
	} else if (dlen_ <= 268435455) {
		header[len++] = ((unsigned char)(dlen_ >> 24) & 0x7f) | 0x80;
		header[len++] = ((unsigned char)(dlen_ >> 16) & 0x7f) | 0x80;
		header[len++] = ((unsigned char)(dlen_ >> 8 ) & 0x7f) | 0x80;
		header[len++] = (unsigned char)  dlen_ & 0x7f;
	} else {
		logger_error("invalid dlen_=%u", dlen_);
		return false;
	}

	out.append(header, len);
	return true;
}

void mqtt_message::pack_add(unsigned char ch, string& out) {
	out += (unsigned char) ch;
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

static struct {
	int status;
	int (mqtt_message::*handler)(const char*, int);
} handlers[] = {
	{ MQTT_STAT_HDR_TYPE,	&mqtt_message::update_header_type	},
	{ MQTT_STAT_HDR_LEN,	&mqtt_message::update_header_len	},
};

int mqtt_message::header_update(const char* data, int dlen) {
	if (data == NULL || dlen <= 0) {
		logger_error("invalid input");
		return -1;
	}

	while (dlen > 0 && !header_finished_) {
		int ret = (this->*handlers[status_].handler)(data, dlen);
		if (ret < 0) {
			return -1;
		}
		data += dlen - ret;
		dlen  = ret;
	}
	return dlen;
}

int mqtt_message::update_header_type(const char* data, int dlen) {
	assert(data && dlen > 0);

	char ch = *data++;
	char type = (ch &0xff) >> 4;
	if (type <= MQTT_RESERVED_MIN || type >= MQTT_RESERVED_MAX) {
		logger_error("invalid type=%d", type);
		return -1;
	}

	type_   = (mqtt_type_t) type;
	hflags_ = ch & 0x0f;
	status_ = MQTT_STAT_HDR_LEN;
	hlen_   = 0;
	return --dlen;
}

int mqtt_message::update_header_len(const char* data, int dlen) {
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

#if 1
	dlen_ = 0;
	for (unsigned i = 0; i < hlen_; i++) {
		dlen_ |= (unsigned) ((hbuf_[i] & 0x7f) << (8 * i));
	}
#else
	if (hlen_ == 1) {
		dlen_ = (unsigned) (hbuf_[0] & 0x7f);
	} else if (hlen_ == 2) {
		dlen_ = (unsigned) (((hbuf_[0] & 0x7f) << 8)
				| (hbuf_[1] & 0x7f));
	} else if (hlen_ == 3) {
		dlen_ = (unsigned) (((hbuf_[0] & 0x7f) << 16)
				| ((hbuf_[1] & 0x7f) << 8)
				| (hbuf_[0] & 0x7f));
	} else if (hlen_ == 4) {
		dlen_ = (unsigned) (((hbuf_[0] & 0x7f) << 24)
				| ((hbuf_[1] & 0x7f) << 16)
				| ((hbuf_[2] & 0x7f) << 8)
				| (hbuf_[0] & 0x7f));
	}
#endif

	header_finished_ = true;
	return dlen;
}

} //namespace acl
