//
// Created by shuxin ¡¡¡¡zheng on 2020/11/14.
//

#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/net/rfc1035.hpp"
#endif

#define STRNCPY	ACL_SAFE_STRNCPY

namespace acl {

rfc1035_request::rfc1035_request(void) {
	name_[0] = 0;
	qid_     = 0;
	type_    = rfc1035_type_null;
}

rfc1035_request::~rfc1035_request(void) {}

rfc1035_request& rfc1035_request::set_name(const char *name) {
	STRNCPY(name_, name, sizeof(name_));
	return *this;
}

rfc1035_request& rfc1035_request::set_qid(unsigned short id) {
	qid_ = id;
	return *this;
}

rfc1035_request& rfc1035_request::set_type(rfc1035_type_t type) {
	type_ = type;
	return *this;
}

size_t rfc1035_request::build_query(char* buf, size_t size) {
	if (name_[0] == 0) {
		logger_error("name not set, call set_name first");
		return 0;
	}
	if (size < 512) {
		logger_error("buf' size(%ld) < 512 too small", (long) size);
		return 0;
	}

	switch (type_) {
	case rfc1035_type_a:
		return acl_rfc1035_build_query4a(name_, buf, size, qid_, NULL);
	case rfc1035_type_aaaa:
		return acl_rfc1035_build_query4aaaa(name_, buf, size, qid_, NULL);
	case rfc1035_type_mx:
		return acl_rfc1035_build_query4mx(name_, buf, size, qid_, NULL);
	default:
		logger_error("type(%d) not supported", (int) type_);
		return 0;
	}
}

size_t rfc1035_request::build_query4ptr(const struct in_addr addr,
	char* buf, size_t size) {
	if (size < 512) {
		logger_error("buf' size(%ld) < 512 too small", (long) size);
		return 0;
	}

	return acl_rfc1035_build_query4ptr(addr, buf, size, qid_, NULL);
}

bool rfc1035_request::parse_request(const void *buf, size_t len) {
	ACL_RFC1035_MESSAGE* message =
		acl_rfc1035_request_unpack((const char*) buf, len);
	if (message == NULL) {
		return false;
	}

	ACL_RFC1035_QUERY *query = message->query;
	if (query == NULL || query->name[0] == 0) {
		acl_rfc1035_message_destroy(message);
		return false;
	}

	qid_  = message->id;
	STRNCPY(name_, query->name, sizeof(name_));

	switch (query->qtype) {
	case ACL_RFC1035_TYPE_A:
		type_ = rfc1035_type_a;
		break;
	case ACL_RFC1035_TYPE_AAAA:
		type_ = rfc1035_type_aaaa;
		break;
	default:
		logger_error("type(%d) not supported", (int) type_);
		acl_rfc1035_message_destroy(message);
		return false;
	}
	acl_rfc1035_message_destroy(message);
	return true;
}

//////////////////////////////////////////////////////////////////////////////

rfc1035_response::rfc1035_response(void) {
	name_[0] = 0;
	qid_     = 0;
	type_    = rfc1035_type_null;
	ttl_     = 0;
}

rfc1035_response::~rfc1035_response(void) {}

rfc1035_response& rfc1035_response::set_name(const char *name) {
	STRNCPY(name_, name, sizeof(name_));
	return *this;
}

rfc1035_response& rfc1035_response::set_qid(unsigned short id) {
	qid_ = id;
	return *this;
}

rfc1035_response& rfc1035_response::set_type(rfc1035_type_t type) {
	type_ = type;
	return *this;
}

rfc1035_response& rfc1035_response::set_ttl(int ttl) {
	ttl_ = ttl;
	return *this;
}

size_t rfc1035_response::build_reply(const std::vector<string>& addrs,
	char *buf, size_t size) {
	if (size < 512) {
		logger_error("buf' size(%ld) < 512 too small", (long) size);
		return 0;
	}

	int type;
	switch (type_) {
	case rfc1035_type_a:
		type = ACL_RFC1035_TYPE_A;
		break;
	case rfc1035_type_aaaa:
		type = ACL_RFC1035_TYPE_A;
		break;
	default:
		logger_error("type(%d) not supported", (int) type_);
		return 0;
	}

	ACL_ARGV* ips = acl_argv_alloc(5);
	for (std::vector<string>::const_iterator cit = addrs.begin();
	     cit != addrs.end(); ++cit) {
		acl_argv_add(ips, (*cit).c_str(), NULL);
	}

	ACL_RFC1035_REPLY reply;

	reply.ip_type = type;
	reply.hostname = name_;
#if 0
	reply.domain_root = ".dns.com";
	reply.dns_name = "ipv6-static.dns.com";
	reply.domain_root = reply.dns_name;
	reply.dns_ip = "127.0.0.1";
#else
	reply.domain_root = NULL;
	reply.dns_name = NULL;
	reply.dns_ip = NULL;
#endif
	reply.ips = ips;
	reply.ttl = ttl_;
	reply.qid = qid_;

	size_t n = acl_rfc1035_build_reply(&reply, buf, size);
	acl_argv_free(ips);

	return n;
}

bool rfc1035_response::parse_reply(const void *buf, size_t len) {
	ACL_RFC1035_MESSAGE* message =
		acl_rfc1035_response_unpack((const char* ) buf, len);

	if (message == NULL) {
		return false;
	}

	qid_ = message->id;

	for (unsigned short i = 0; i < message->ancount; i++) {
		if (message->answer[i].type == ACL_RFC1035_TYPE_A) {
			struct sockaddr_in in;
			size_t n = message->answer[i].rdlength > 4
				   ? 4 : message->answer[i].rdlength;

			memcpy(&in.sin_addr, message->answer[i].rdata, n);
			in.sin_family = AF_INET;

			char ip[64];
			if (acl_inet_ntop((const struct sockaddr*) &in,
					  ip, sizeof(ip))) {
				addrs4a_.push_back(ip);
			}
#ifdef	AF_INET6
		} else if (message->answer[i].type == ACL_RFC1035_TYPE_AAAA) {
			struct sockaddr_in6 in;
			size_t n = message->answer[i].rdlength > 16
				   ? 16 : message->answer[i].rdlength;

			memcpy(&in.sin6_addr, message->answer[i].rdata, n);
			in.sin6_family = AF_INET6;

			char ip[64];
			if (acl_inet_ntop((const struct sockaddr*) &in,
					  ip, sizeof(ip))) {
				addrs4aaaa_.push_back(ip);
			}
#endif
		} else if (message->answer[i].type == ACL_RFC1035_TYPE_CNAME) {
			char cname[256];
			len = sizeof(cname) - 1;
			if (len > message->answer[i].rdlength) {
				len = message->answer[i].rdlength;
			}
			memcpy(cname, message->answer[i].rdata, len);
			cname[len] = 0;
			cnames_.push_back(cname);
			continue;
		} else {
			continue;
		}

		if (name_[0] == 0 && message->answer[i].name[0] != 0) {
			STRNCPY(name_, message->answer[i].name, sizeof(name_));
		}
	}

	acl_rfc1035_message_destroy(message);
	return true;
}

} // namespace acl
