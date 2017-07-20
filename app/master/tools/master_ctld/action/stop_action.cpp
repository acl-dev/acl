#include "stdafx.h"
#include "stop_action.h"

int stop_action::run(acl::string& out)
{
	stop_req_t req;
	stop_res_t res;
	return forward<stop_req_t, stop_res_t>(req, res, out);
}
