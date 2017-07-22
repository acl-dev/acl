#pragma once

class base_action
{
public:
	base_action(const acl::string&, acl::HttpServletRequest&,
		acl::HttpServletResponse&);
	virtual ~base_action(void) {}

	virtual int run(acl::string& json) = 0;

protected:
	acl::string addr_;
	acl::HttpServletRequest& req_;
	acl::HttpServletResponse& res_;

	acl::json* get_json(void);
};
