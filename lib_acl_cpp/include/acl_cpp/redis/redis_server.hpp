#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <map>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class redis_client;

class ACL_CPP_API redis_server : public redis_command
{
public:
	redis_server(redis_client* conn = NULL);
	~redis_server();

	/////////////////////////////////////////////////////////////////////

	bool bgsave();
	bool client_getname(string& buf);
	bool client_kill(const char* addr);
	int client_list(string& buf);
	bool client_setname(const char* name);
	int config_get(const char* parameter, std::map<string, string>& out);
	bool config_resetstat();
	bool config_rewrite();
	bool config_set(const char* name, const char* value);
	int dbsize();
	bool flushall();
	bool flushdb();
	int info(string& buf);
	time_t lastsave();

	bool monitor();
	bool get_command(string& buf);

	bool save();
	void shutdown(bool save_data = true);
	bool slaveof(const char* ip, int port);
	bool get_time(time_t& stamp, int& escape);
};

} // namespace acl
