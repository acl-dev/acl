#pragma once
#include <vector>
#include "acl_cpp/stdlib/string.hpp"

namespace acl {

class redis_key
{
public:
	redis_key() {}
	~redis_key() {}

#ifdef	WIN32
	bool del_keys(const char* first_key, ...);
#else
	bool del_keys(const char* first_key, ...)
		__attribute__((format(printf, 2, 3)));
#endif
	int  get_keys(const char* pattern, std::vector<acl::string>& out);
	bool if_exists(const char* key);
	redis_key_t get_key_type(const char* key);
	bool set_ttl(const char* key, int n);
	int  get_ttl(const char* key);
	bool set_expireat(const char* key, time_t stamp);
	bool set_persist(const char*);
};

} // namespace acl
