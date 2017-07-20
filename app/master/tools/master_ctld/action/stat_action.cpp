#include "stdafx.h"
#include "stat_action.h"

int stat_action::run(acl::string& out)
{
	stat_req_t req;
	stat_res_t res;
	return forward<stat_req_t, stat_res_t>(req, res, out);
}
