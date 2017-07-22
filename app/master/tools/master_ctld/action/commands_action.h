#pragma once
#include "base_action.h"

class commands_action : public base_action
{
public:
	commands_action(const char* addr, acl::HttpServletRequest&,
		acl::HttpServletResponse&, const char* cmd);
	~commands_action(void) {}

	// @override
	int run(acl::string& out);

private:
	acl::string cmd_;
};
