#pragma once

#include "base_action.h"

class list_action : public base_action
{
public:
	list_action(void) {}
	~list_action(void) {}

protected:
	// @override
	int run(acl::string&);
};
