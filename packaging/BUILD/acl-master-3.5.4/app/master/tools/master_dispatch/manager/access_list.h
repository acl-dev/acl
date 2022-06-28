#pragma once
#include <vector>

// 用户访问列表类，用来判断所给用户是否允许使用本服务

class access_list : public acl::singleton<access_list>
{
public:
	access_list();
	~access_list();

	void set_allow_users(const char* whitelist);
	bool check_user(const char* user);

	void set_allow_clients(const char* iplist);
	bool check_client(const char* ip);

	void set_allow_servers(const char* iplist);
	bool check_server(const char* ip);

private:
	bool allow_all_;
	std::vector<acl::string> white_list_;
	ACL_IPLINK* allow_clients_;
	ACL_IPLINK* allow_servers_;

	void add_user(const char* user);
};
