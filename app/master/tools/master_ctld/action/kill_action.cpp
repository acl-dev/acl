#include "stdafx.h"
#include "kill_action.h"

int kill_action::run(acl::string& out)
{
	kill_req_t req;
	kill_res_t res;
	return forward<kill_req_t, kill_res_t>(req, res, out);
}
