#pragma once

#include "base_action.h"

class reload_action : public base_action
{
public:
	reload_action(void) {}
	~reload_action(void) {}

protected:
	// @override
	int run(acl::string&);
};
