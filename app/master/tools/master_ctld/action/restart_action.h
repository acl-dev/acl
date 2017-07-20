#pragma once

#include "base_action.h"

class restart_action : public base_action
{
public:
	restart_action(void) {}
	~restart_action(void) {}

protected:
	// @override
	int run(acl::string&);
};
