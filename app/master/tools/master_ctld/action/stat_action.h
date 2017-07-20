#pragma once

#include "base_action.h"

class stat_action : public base_action
{
public:
	stat_action(void) {}
	~stat_action(void) {}

protected:
	// @override
	int run(acl::string&);
};
