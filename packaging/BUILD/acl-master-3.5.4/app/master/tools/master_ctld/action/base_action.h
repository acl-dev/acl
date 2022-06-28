#pragma once

class base_action
{
public:
	base_action(const acl::string&, acl::HttpServletRequest&,
		acl::HttpServletResponse&);
	virtual ~base_action(void) {}

	virtual int run(acl::string& json) = 0;

	void set_conf(const char* path);

protected:
	acl::string addr_;
	acl::string conf_;
	acl::HttpServletRequest& req_;
	acl::HttpServletResponse& res_;

	acl::json* get_json(void);
};
