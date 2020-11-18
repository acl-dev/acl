//
// Created by shuxin ¡¡¡¡zheng on 2020/11/14.
//

#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include <vector>

struct ACL_RFC1035_MESSAGE;

namespace acl {

typedef enum {
	rfc1035_type_null,
	rfc1035_type_a,
	rfc1035_type_aaaa,
	rfc1035_type_mx,
	rfc1035_type_txt,
	rfc1035_type_ptr,
} rfc1035_type_t;

class ACL_CPP_API rfc1035_request {
public:
	rfc1035_request(void);
	~rfc1035_request(void);

	rfc1035_request& set_name(const char* name);
	rfc1035_request& set_qid(unsigned short id);
	rfc1035_request& set_type(rfc1035_type_t type);

	const char* get_name(void) const {
		return name_;
	}

	unsigned short get_qid(void) const {
		return qid_;
	}

	rfc1035_type_t get_type(void) const {
		return type_;
	}

public:
	size_t build_query(char* buf, size_t size);
	size_t build_query4ptr(const struct in_addr addr, char* buf, size_t size);
	bool parse_request(const void* buf, size_t len);

private:
	char name_[256];
	unsigned short qid_;
	rfc1035_type_t type_;
};

class ACL_CPP_API rfc1035_response {
public:
	rfc1035_response(void);
	~rfc1035_response(void);

	rfc1035_response& set_name(const char* name);
	rfc1035_response& set_qid(unsigned short id);
	rfc1035_response& set_type(rfc1035_type_t type);
	rfc1035_response& set_ttl(int ttl);

	const char* get_name(void) const {
		return name_;
	}

	unsigned short get_qid(void) const {
		return qid_;
	}

	rfc1035_type_t get_type(void) const {
		return type_;
	}

	int get_ttl(void) const {
		return ttl_;
	}

public:
	size_t build_reply(const std::vector<string>& addrs,
		char* buf, size_t size);
	bool parse_reply(const void* buf, size_t len);

private:
	char name_[256];
	unsigned short qid_;
	rfc1035_type_t type_;
	int ttl_;
	std::vector<string> addrs4a_;
	std::vector<string> addrs4aaaa_;
	std::vector<string> cnames_;
};

} // namespace acl
