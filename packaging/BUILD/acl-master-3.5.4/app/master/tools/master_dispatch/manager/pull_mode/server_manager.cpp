#include "stdafx.h"
#include "pull_mode/server_manager.h"

bool server_manager::init(const char* server_list)
{
	char* buf = acl_mystrdup(server_list);
	char* addrs = acl_mystr_trim(buf);
	ACL_ARGV* tokens = acl_argv_split(addrs, ";, ");
	ACL_ITER iter;
	acl::string addr;
	acl_foreach(iter, tokens)
	{
		const char* ptr = (const char*) iter.data;
		if (get_addr(ptr, addr) == false)
		{
			logger_error("invalid server addr: %s", addr.c_str());
			continue;
		}
		if (add_addr(addr.c_str()) == true)
			logger("add one service: %s", addr.c_str());
	}
	acl_argv_free(tokens);
	acl_myfree(buf);

	return !servers_.empty();
}

bool server_manager::get_addr(const char* addr, acl::string& buf)
{
	// 数据格式：IP:PORT[:CONNECT_COUNT]
	ACL_ARGV* tokens = acl_argv_split(addr, ":|");
	if (tokens->argc < 2)
	{
		logger_error("invalid addr: %s", addr);
		acl_argv_free(tokens);
		return false;
	}

	int port = atoi(tokens->argv[1]);
	if (port <= 0 || port >= 65535)
	{
		logger_error("invalid addr: %s, port: %d", addr, port);
		acl_argv_free(tokens);
		return false;
	}
	buf.format("%s:%d", tokens->argv[0], port);
	acl_argv_free(tokens);
	return true;
}

bool server_manager::add_addr(const char* addr)
{
	char key[256];
	ACL_SAFE_STRNCPY(key, addr, sizeof(key));
	acl_lowercase(key);

	std::vector<acl::string>::iterator it = servers_.begin();
	for (; it != servers_.end(); ++it)
	{
		if (strcasecmp(key, (*it).c_str()) == 0)
			return false;
	}

	servers_.push_back(key);
	return true;
}
