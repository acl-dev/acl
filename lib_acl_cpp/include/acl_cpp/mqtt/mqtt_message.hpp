#pragma once

#include "../stdlib/string.hpp"
#include "mqtt_header.hpp"

namespace acl {

class ACL_CPP_API mqtt_message {
public:
	mqtt_message(mqtt_type_t type);
	mqtt_message(const mqtt_header& header);
	virtual ~mqtt_message(void);

public:
	virtual bool to_string(string& out) {
		(void) out;
		return false;
	}

	virtual int update(const char* data, int dlen) {
		(void) data;
		return dlen;
	}

	virtual bool is_finished(void) const {
		return false;
	}

	mqtt_header& get_header(void) {
		return header_;
	}

	const mqtt_header& get_header(void) const {
		return header_;
	}

public:
	static mqtt_message* create_message(const mqtt_header& header);

protected:
	mqtt_header header_;

	void pack_add(unsigned char ch, string& out);
	void pack_add(unsigned short n, string& out);
	void pack_add(const string& s, string& out);

	bool unpack_short(const char* data, size_t len, unsigned short& out);
};

} // namespace acl
