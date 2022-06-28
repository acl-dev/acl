#pragma once

class allow_list : public acl::singleton<allow_list>
{
public:
	allow_list();
	~allow_list();

	void set_allow_manager(const char* white_list);
	bool allow_manager(const char* ip);

private:
	ACL_IPLINK* manager_allow_;
};
