#include "acl_stdafx.hpp"
#include "acl_cpp/mqtt/mqtt_message.hpp"

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

enum {
	MQTT_STAT_HDR_TYPE,
	MQTT_STAT_HDR_LEN,
};

mqtt_message::mqtt_message(mqtt_type_t type)
: status_(MQTT_STAT_HDR_TYPE)
, type_(type)
, header_finished_(false)
, hflags_(0)
, dlen_(0)
{
	assert(type > MQTT_CONNACK && type < MQTT_RESERVED_MAX);
	if (__constrains[type].id == MQTT_NEED) {
		hlen_ = 2;
	} else {
		hlen_ = 0;
	}
}

mqtt_message::~mqtt_message(void) {}

void mqtt_message::set_payload_length(unsigned len) {
	if (len == 0) {
		return;
	}

	if (__constrains[type_].payload != MQTT_NEED) {
		logger_error("%s: needn't payload", mqtt_type_desc(type_));
	} else {
		dlen_ = len;
	}
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
	int (mqtt_message::*handler)(const char*, unsigned);
} handlers[] = {
	{ MQTT_STAT_STR_LEN,	&mqtt_message::unpack_string_len	},
	{ MQTT_STAT_STR_VAL,	&mqtt_message::unpack_string_val	},
	{ MQTT_STAT_HDR_TYPE,	&mqtt_message::unpack_header_type	},
	{ MQTT_STAT_HDR_LEN,	&mqtt_message::unpack_header_len	},
};

int mqtt_message::header_update(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		logger_error("invalid input");
		return -1;
	}

	while (dlen > 0 && !header_finished_) {
		int ret = (this->*handlers[status_].handler)(data, dlen);
		if (ret < 0) {
			return -1;
		}
		dlen = (unsigned) ret;
		data += ret;
	}
	return dlen;
}

int mqtt_message::unpack_header_type(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		return 0;
	}

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

int mqtt_message::unpack_header_len(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		return 0;
	}

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

	status_ = MQTT_STAT_HDR_VAR;

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

	return dlen;
}

#define	HDR_LEN_LEN	2

int mqtt_message::unpack_string_len(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		return 0;
	}

	for (; hlen_ < HDR_LEN_LEN && dlen > 0;) {
		hbuf_[hlen_++] = *data++;
		dlen--;
	}

	if (hlen_ < HDR_LEN_LEN) {
		assert(dlen == 0);
		return dlen;
	}

	if (!unpack_short(&hbuf_[0], 2, vlen_)) {
		logger_error("unpack cid len error");
		return -1;
	}

	status_ = MQTT_STAT_STR_VAL;
	return dlen;
}

int mqtt_message::unpack_string_val(const char* data, unsigned dlen) {
	assert(buff_->size() <= (size_t) vlen_);

	size_t i, left = (size_t) vlen_ - buff_->size();
	for (i = 0; i < left && dlen > 0; i++) {
		(*buff_) += *data++;
		dlen--;
	}

	if (buff_->size() == (size_t) vlen_) {
		status_ = status_next_;
		return dlen;
	}
	assert(dlen == 0);
	return dlen;
}

void mqtt_message::unpack_string_await(string& out, int next) {
	out.clear();
	buff_        = &out;
	status_next_ = next;
	hlen_        = 0;
	status_      = MQTT_STAT_STR_LEN;
}

} //namespace acl
