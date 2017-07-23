#pragma once
#include <vector>

class server_manager : public acl::singleton<server_manager>
{
public:
	server_manager() {}
	~server_manager() {}

	bool init(const char* server_list);

	const std::vector<acl::string>& get_addrs() const
	{
		return servers_;
	}

private:
	std::vector<acl::string> servers_;

	bool get_addr(const char* addr, acl::string& buf);
	bool add_addr(const char* addr);
};
