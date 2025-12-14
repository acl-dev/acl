#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include "../stdlib/string.hpp"
#include "session.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl
{

class redis;
class redis_client_cluster;

class ACL_CPP_API redis_session : public session
{
public:
	redis_session(redis_client_cluster& cluster,
		time_t ttl = 0, const char* sid = NULL);
	~redis_session();

	// Base class virtual function, set hash attribute value to redis server
	bool set(const char* name, const char* value);

	// Base class virtual function, set hash attribute value to redis server
	bool set(const char* name, const void* value, size_t len);

	// Base class virtual function, get corresponding attribute value from hash object on redis server
	const session_string* get_buf(const char* name);

	// Base class virtual function, delete an attribute value from hash object on redis server
	bool del(const char* name);

	// Base class pure virtual function, delete data from redis
	bool remove();

	// Base class pure virtual function, get data from redis
	bool get_attrs(std::map<string, session_string>& attrs);

	// Base class virtual function, get data from redis
	bool get_attrs(const std::vector<string>& names,
		std::vector<session_string>& values);

	// Base class pure virtual function, add or modify data in redis
	bool set_attrs(const std::map<string, session_string>& attrs);

protected:
	// Reset the cache time of session on redis
	bool set_timeout(time_t ttl);

private:
	redis_client_cluster& cluster_;
	redis* command_;
	// size_t max_conns_;
	std::map<string, session_string*> buffers_;
};

} // namespace acl

#endif // ACL_CLIENT_ONLY
