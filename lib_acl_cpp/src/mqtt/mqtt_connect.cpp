#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_connect.hpp"
#endif

namespace acl {

enum {
	MQTT_STAT_HDR_VAR,
	MQTT_STAT_CID_LEN,
	MQTT_STAT_CID_VAL,
	MQTT_STAT_USERNAME_LEN,
	MQTT_STAT_USERNAME_VAL,
	MQTT_STAT_PASSWD_LEN,
	MQTT_STAT_PASSWD_VAL,
	MQTT_STAT_WILL_TOPIC_LEN,
	MQTT_STAT_WILL_TOPIC_VAL,
	MQTT_STAT_WILL_MSG_LEN,
	MQTT_STAT_WILL_MSG_VAL,
};

mqtt_connect::mqtt_connect(void)
: mqtt_message(MQTT_CONNECT)
, finished_(false)
, dlen_(0)
, will_qos_(MQTT_QOS0)
, conn_flags_(0)
, keep_alive_(300)
{
	status_ = MQTT_STAT_HDR_VAR;  // just for update()
}

mqtt_connect::mqtt_connect(const mqtt_header& header)
: mqtt_message(header)
, finished_(false)
, dlen_(0)
, will_qos_(MQTT_QOS0)
, conn_flags_(0)
, keep_alive_(300)
{
	status_ = MQTT_STAT_HDR_VAR;  // just for update()
}

mqtt_connect::~mqtt_connect(void) {}

void mqtt_connect::set_keep_alive(unsigned short keep_alive) {
	keep_alive_ = keep_alive;
}

void mqtt_connect::set_cid(const char* cid) {
	if (cid && *cid) {
		cid_ = cid;
	}
}

void mqtt_connect::set_username(const char* name) {
	if (name && *name) {
		username_ = name;
	}
}

void mqtt_connect::set_passwd(const char* passwd) {
	if (passwd && *passwd) {
		passwd_ = passwd;
	}
}

void mqtt_connect::set_will_qos(mqtt_qos_t qos) {
	will_qos_ = qos;

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

void mqtt_connect::clean_session(void) {
	conn_flags_ |= 0x02;
}

bool mqtt_connect::has_session(void) const {
	return conn_flags_ & 0x02 ? true : false;
}

bool mqtt_connect::to_string(string& out) {
	unsigned len = 10;  // length of the body's header

	len += 2;
	len += (unsigned) cid_.size();
	if (!username_.empty()) {
		conn_flags_ |= 1 << 7;
		len += 2;
		len += (unsigned) username_.size();
	}
	if (!passwd_.empty()) {
		conn_flags_ |= 1 << 6;
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

	mqtt_header& header = this->get_header();
	header.set_remaing_length(len);

	if (!header.build_header(out)) {
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

	if (!will_topic_.empty() && !will_msg_.empty()) {
		this->pack_add(will_topic_, out);
		this->pack_add(will_msg_, out);
	}

	if (!username_.empty()) {
		this->pack_add(username_, out);
	}

	if (!passwd_.empty()) {
		this->pack_add(passwd_, out);
	}

	return true;
}

static struct {
	int status;
	int (mqtt_connect::*handler)(const char*, int);
} handlers[] = {
	{ MQTT_STAT_HDR_VAR,		&mqtt_connect::update_header_var    },

	{ MQTT_STAT_CID_LEN,		&mqtt_connect::update_cid_len       },
	{ MQTT_STAT_CID_VAL,		&mqtt_connect::update_cid_val       },
	{ MQTT_STAT_USERNAME_LEN,	&mqtt_connect::update_username_len  },
	{ MQTT_STAT_USERNAME_VAL,	&mqtt_connect::update_username_val  },
	{ MQTT_STAT_PASSWD_LEN,		&mqtt_connect::update_passwd_len    },
	{ MQTT_STAT_PASSWD_VAL,		&mqtt_connect::update_passwd_val    },
	{ MQTT_STAT_WILL_TOPIC_LEN,	&mqtt_connect::update_will_topic_len},
	{ MQTT_STAT_WILL_TOPIC_VAL,	&mqtt_connect::update_will_topic_val},
	{ MQTT_STAT_WILL_MSG_LEN,	&mqtt_connect::update_will_msg_len  },
	{ MQTT_STAT_WILL_MSG_LEN,	&mqtt_connect::update_will_msg_len  },
	{ MQTT_STAT_WILL_MSG_VAL,	&mqtt_connect::update_will_msg_val  },
};

int mqtt_connect::update(const char* data, int dlen) {
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

#define	HDR_VAR_LEN	10

int mqtt_connect::update_header_var(const char* data, int dlen) {
	assert(data && dlen > 0);
	assert(sizeof(buff_) >= HDR_VAR_LEN);

	if (dlen_ >= HDR_VAR_LEN) {
		logger_error("invalid header var");
		return -1;
	}

	for (; dlen_ < HDR_VAR_LEN && dlen > 0;) {
		buff_[dlen_++] = *data++;
		dlen--;
	}

	if (dlen_ < HDR_VAR_LEN) {
		assert(dlen == 0);
		return dlen;
	}

	char label[5];
	label[0] = buff_[2];
	label[1] = buff_[3];
	label[2] = buff_[4];
	label[3] = buff_[5];
	label[4] = 0;
	if (strcasecmp(label, "MQTT") != 0) {
		logger_warn("invalid label: %s", label);
	}
	conn_flags_ = buff_[7];

	if (!this->unpack_short(&buff_[9], 2, keep_alive_)) {
		logger_error("unpack keep_alive error");
		return -1;
	}

	dlen_   = 0;
	status_ = MQTT_STAT_CID_LEN;
	return dlen;
}

#define HDR_LEN_LEN	2

#define	SAVE_LENGTH(x) do { \
	assert(data && dlen > 0); \
	unsigned short n; \
	for (; x < HDR_LEN_LEN && dlen > 0;) { \
		buff_[x++] = *data++; \
		dlen--; \
	} \
	if (x < HDR_LEN_LEN) { \
		assert(dlen == 0); \
		return dlen; \
	} \
	if (!this->unpack_short(&buff_[0], 2, n)) { \
		logger_error("unpack cid len error"); \
		return -1; \
	} \
	x = n; \
} while (0)

int mqtt_connect::update_cid_len(const char* data, int dlen) {
	SAVE_LENGTH(dlen_);

	if (dlen_ > 0) {
		status_ = MQTT_STAT_CID_VAL;
	} else if ((conn_flags_ & 0x80)) {
		status_ = MQTT_STAT_USERNAME_LEN;
	} else if ((conn_flags_ & 0x40)) {
		status_ = MQTT_STAT_PASSWD_LEN;
	} else if ((conn_flags_ & 0x04)) {
		status_ = MQTT_STAT_WILL_TOPIC_LEN;
	} else {
		finished_ = true;
	}

	return dlen;
}

#define SAVE_DATA(buff) do { \
	assert(data && dlen > 0 && dlen_ > 0); \
	for (; dlen_ > 0 && dlen > 0;) { \
		buff += *data++; \
		--dlen_; \
		--dlen; \
	} \
	if (dlen_ > 0) { \
		assert(dlen == 0); \
		return dlen; \
	} \
} while (0)

int mqtt_connect::update_cid_val(const char* data, int dlen) {
	SAVE_DATA(cid_);

	if ((conn_flags_ & 0x80)) {
		status_ = MQTT_STAT_USERNAME_LEN;
	} else if ((conn_flags_ & 0x40)) {
		status_ = MQTT_STAT_PASSWD_LEN;
	} else if ((conn_flags_ & 0x04)) {
		status_ = MQTT_STAT_WILL_TOPIC_LEN;
	} else {
		finished_ = true;
	}

	return dlen;
}

int mqtt_connect::update_username_len(const char* data, int dlen) {
	SAVE_LENGTH(dlen_);

	if (dlen_ > 0) {
		status_ = MQTT_STAT_USERNAME_VAL;
	} else if ((conn_flags_ & 0x40)) {
		status_ = MQTT_STAT_PASSWD_LEN;
	} else if ((conn_flags_ & 0x04)) {
		status_ = MQTT_STAT_WILL_TOPIC_LEN;
	} else {
		finished_ = true;
		return dlen;
	}

	return dlen;
}

int mqtt_connect::update_username_val(const char* data, int dlen) {
	SAVE_DATA(username_);

	if ((conn_flags_ & 0x40)) {
		status_ = MQTT_STAT_PASSWD_LEN;
	} else if ((conn_flags_ & 0x04)) {
		status_ = MQTT_STAT_WILL_TOPIC_LEN;
	} else {
		finished_ = true;
	}

	return dlen;
}

int mqtt_connect::update_passwd_len(const char* data, int dlen) {
	SAVE_LENGTH(dlen_);

	if (dlen_ > 0) {
		status_ = MQTT_STAT_PASSWD_VAL;
	} else if ((conn_flags_ & 0x04)) {
		status_ = MQTT_STAT_WILL_TOPIC_LEN;
	} else {
		finished_ = true;
	}

	return dlen;
}

int mqtt_connect::update_passwd_val(const char* data, int dlen) {
	SAVE_DATA(passwd_);

	if ((conn_flags_ & 0x04)) {
		status_ = MQTT_STAT_WILL_TOPIC_LEN;
	} else {
		finished_ = true;
	}

	return dlen;
}

int mqtt_connect::update_will_topic_len(const char* data, int dlen) {
	SAVE_LENGTH(dlen_);

	status_ = MQTT_STAT_WILL_TOPIC_VAL;
	return dlen;
}

int mqtt_connect::update_will_topic_val(const char* data, int dlen) {
	SAVE_DATA(will_topic_);

	status_ = MQTT_STAT_WILL_MSG_LEN;
	return dlen;
}

int mqtt_connect::update_will_msg_len(const char* data, int dlen) {
	SAVE_LENGTH(dlen_);

	if (dlen_ > 0) {
		status_ = MQTT_STAT_WILL_MSG_VAL;
	} else {
		finished_ = true;
	}

	return dlen;
}

int mqtt_connect::update_will_msg_val(const char* data, int dlen) {
	SAVE_DATA(will_msg_);

	finished_ = true;
	return dlen;
}

} // namespace acl
