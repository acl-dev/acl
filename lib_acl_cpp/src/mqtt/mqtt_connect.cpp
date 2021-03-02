#include "acl_stdafx.hpp"
#include "acl_cpp/mqtt/mqtt_connect.hpp"

namespace acl {

mqtt_connect::mqtt_connect(void)
: mqtt_message(MQTT_CONNACK)
, finished_(false)
, hlen_(0)
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
	MQTT_STAT_DONE,
};

static struct {
	int status;
	int (mqtt_connect::*handler)(const char*, unsigned);
} connect_handlers[] = {
	{ MQTT_STAT_STR_LEN,	&mqtt_message::unpack_string_len	},
	{ MQTT_STAT_STR_VAL,	&mqtt_message::unpack_string_val	},

	{ MQTT_STAT_HDR_VAR,	&mqtt_connect::unpack_header_var	},

	{ MQTT_STAT_CID,	&mqtt_connect::unpack_cid		},
	{ MQTT_STAT_USERNAME,	&mqtt_connect::unpack_username		},
	{ MQTT_STAT_PASSWD,	&mqtt_connect::unpack_passwd		},
	{ MQTT_STAT_WILL_TOPIC,	&mqtt_connect::unpack_will_topic	},
	{ MQTT_STAT_WILL_MSG,	&mqtt_connect::unpack_will_msg		},
	{ MQTT_STAT_DONE,	&mqtt_connect::unpack_done		},
};

int mqtt_connect::update(const char* data, unsigned dlen) {
	if (data == NULL || dlen == 0) {
		logger_error("invalid input");
		return -1;
	}

	while (dlen > 0 && !finished_) {
		int ret = (this->*connect_handlers[status_].handler)(data, dlen);
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

	int next = MQTT_STAT_DONE;

	this->unpack_string_await(will_msg_, next);
	return dlen;
}

int mqtt_connect::unpack_done(const char*, unsigned dlen) {
	finished_ = true;
	return dlen;
}

} // namespace acl
