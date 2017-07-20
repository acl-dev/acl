#pragma once

#include "base_action.h"

class stop_action : public base_action
{
public:
	stop_action(void) {}
	~stop_action(void) {}

protected:
	// @override
	int run(acl::string&);
};
