#pragma once

#include "base_action.h"

class start_action : public base_action
{
public:
	start_action(void) {}
	~start_action(void) {}

protected:
	// @override
	int run(acl::string&);
};
