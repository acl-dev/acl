#pragma once

#include "base_action.h"

class kill_action : public base_action
{
public:
	kill_action(void) {}
	~kill_action(void) {}

protected:
	// @override
	int run(acl::string&);
};
