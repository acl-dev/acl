#pragma once
#include <vector>
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/redis/redis_response.hpp"

namespace acl {

class redis_client
{
public:
	redis_client(const char* addr, int conn_timeout = 60, int rw_timeout = 30,
		bool retry = true);
	~redis_client();

	bool open();
	void close();
	const std::vector<redis_response*>& request(const char* cmd,
		const void* data, size_t len);
	void clear();
private:
	socket_stream conn_;
	char* addr_;
	int   conn_timeout_;
	int   rw_timeout_;
	bool  retry_;
	std::vector<redis_response*> res_;
};

} // end namespace acl
