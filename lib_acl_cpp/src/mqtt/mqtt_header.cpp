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
: status_(MQTT_STAT_HDR_TYPE)
, type_(type)
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

void mqtt_message::set_data_length(unsigned len) {
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

	header[0] = ((((char) type_) << 4) & 0xff) | hflags_;
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

bool mqtt_message::unpack_char(const char* in, size_t len, unsigned char& out) {
	if (len < 1) {
		logger_error("too short: %ld", (long) len);
		return false;
	}
	out = (unsigned char) in[0];
	return true;
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

int mqtt_message::unpack_header(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		logger_error("input invalid");
		return -1;
	}
	switch (status_) {
	case MQTT_STAT_HDR_TYPE:
		return unpack_header_type(data, dlen);
	case MQTT_STAT_HDR_LEN:
		return unpack_header_len(data, dlen);
	default:
		return dlen;
	}
}

void mqtt_message::unpack_string_await(string& out, int next) {
	out.clear();
	buff_        = &out;
	status_next_ = next;
	hlen_        = 0;
	status_      = MQTT_STAT_STR_LEN;
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

mqtt_connect::mqtt_connect(void)
: mqtt_message(MQTT_CONNACK)
, hlen_(0)
, finished_(false)
, qos_(MQTT_QOS0)
, conn_flags_(0)
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
		conn_flags_ |= 1 << 7;
	}
}

void mqtt_connect::set_passwd(const char* passwd) {
	if (passwd && *passwd) {
		passwd_ = passwd;
		conn_flags_ |= 1 << 6;
	}
}

void mqtt_connect::set_qos(mqtt_qos_t qos) {
	qos_ = qos;

	switch (qos) {
	case MQTT_QOS0:
		conn_flags_ &= ~(1 << 4 | 1 << 3);
		break;
	case MQTT_QOS1:
		conn_flags_ &= ~(1 << 4);
		conn_flags_ |= 1 << 3;
		break;
	case MQTT_QOS2:
		conn_flags_ &= ~(1 << 3);
		conn_flags_ |= 1 << 4;
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
		conn_flags_ |= 1 << 2;

		len += 2;
		len += (unsigned) will_topic_.size();

		len += 2;
		len += (unsigned) will_msg_.size();
	} else {
		will_topic_.clear();
		will_msg_.clear();
	}

	this->set_data_length(len);
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
	this->pack_add((unsigned char) conn_flags_, out);
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

enum {
	MQTT_STAT_CID = MQTT_STAT_HDR_END,
	MQTT_STAT_USERNAME,
	MQTT_STAT_PASSWD,
	MQTT_STAT_WILL_TOPIC,
	MQTT_STAT_WILL_MSG,
	MQTT_STAT_END,
};

static struct {
	int status;
	int (mqtt_connect::*handler)(const char*, unsigned);
} handlers[] = {
	{ MQTT_STAT_HDR_TYPE,	&mqtt_connect::unpack_header_type	},
	{ MQTT_STAT_HDR_LEN,	&mqtt_connect::unpack_header_len	},
	{ MQTT_STAT_HDR_VAR,	&mqtt_connect::unpack_header_var	},
	{ MQTT_STAT_STR_LEN,	&mqtt_connect::unpack_string_len	},
	{ MQTT_STAT_STR_VAL,	&mqtt_connect::unpack_string_val	},

	{ MQTT_STAT_CID,	&mqtt_connect::unpack_cid		},
	{ MQTT_STAT_USERNAME,	&mqtt_connect::unpack_username		},
	{ MQTT_STAT_PASSWD,	&mqtt_connect::unpack_passwd		},
	{ MQTT_STAT_WILL_TOPIC,	&mqtt_connect::unpack_will_topic	},
	{ MQTT_STAT_WILL_MSG,	&mqtt_connect::unpack_will_msg		},
	{ MQTT_STAT_END,	NULL					},
};

int mqtt_connect::update(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		logger_error("invalid input");
		return -1;
	}

	while (dlen > 0 && !finished_) {
		int ret = (this->*handlers[status_].handler)(data, dlen);
		if (ret < 0) {
			return -1;
		}
		dlen = (unsigned) ret;
		data += ret;
	}
	return dlen;
}

#define	HDR_VAR_LEN	10

int mqtt_connect::unpack_header_var(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		return 0;
	}
	assert(sizeof(hbuf_) >= HDR_VAR_LEN);

	if (hlen_ >= HDR_VAR_LEN) {
		logger_error("invalid header var");
		return -1;
	}

	for (; hlen_ < HDR_VAR_LEN && dlen > 0;) {
		hbuf_[hlen_++] = *data++;
		dlen--;
	}

	if (hlen_ < HDR_VAR_LEN) {
		assert(dlen == 0);
		return dlen;
	}

	char label[5];
	label[0] = hbuf_[2];
	label[1] = hbuf_[3];
	label[2] = hbuf_[4];
	label[3] = hbuf_[5];
	label[4] = 0;
	if (strcasecmp(label, "MQTT") != 0) {
		logger_warn("invalid label: %s", label);
	}
	conn_flags_ = hbuf_[7];
	if (!this->unpack_short(&hbuf_[9], 2, keep_alive_)) {
		logger_error("unpack keep_alive error");
		return -1;
	}

	status_ = MQTT_STAT_CID;
	hlen_   = 0;
	return dlen;
}

int mqtt_connect::unpack_cid(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		return 0;
	}

	int next;
	if ((conn_flags_ & 0x80)) {
		next = MQTT_STAT_USERNAME;
	} else if ((conn_flags_ & 0x40)) {
		next = MQTT_STAT_PASSWD;
	} else if ((conn_flags_ & 0x04)) {
		next = MQTT_STAT_WILL_TOPIC;
	} else {
		finished_ = true;
		return dlen;
	}

	this->unpack_string_await(cid_, next);
	return dlen;
}

int mqtt_connect::unpack_username(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		return 0;
	}

	int next;
	if ((conn_flags_ & 0x40)) {
		next = MQTT_STAT_PASSWD;
	} else if ((conn_flags_ & 0x04)) {
		next = MQTT_STAT_WILL_TOPIC;
	} else {
		finished_ = true;
		return dlen;
	}

	this->unpack_string_await(username_, next);
	return dlen;
}

int mqtt_connect::unpack_passwd(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		return 0;
	}

	int next;
	if ((conn_flags_ & 0x04)) {
		next = MQTT_STAT_WILL_TOPIC;
	} else {
		finished_ = true;
		return dlen;
	}

	this->unpack_string_await(passwd_, next);
	return dlen;
}

int mqtt_connect::unpack_will_topic(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		return 0;
	}

	int next = MQTT_STAT_WILL_MSG;

	this->unpack_string_await(will_topic_, next);
	return dlen;
}

int mqtt_connect::unpack_will_msg(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		return 0;
	}

	int next = MQTT_STAT_WILL_MSG;

	this->unpack_string_await(will_msg_, next);
	return dlen;
}

} //namespace acl
