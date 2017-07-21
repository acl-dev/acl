#include "stdafx.h"
#include "restart_action.h"

int restart_action::run(acl::string& out)
{
	restart_req_t req;
	restart_res_t res;
	return forward<restart_req_t, restart_res_t>(req, res, out);
}
