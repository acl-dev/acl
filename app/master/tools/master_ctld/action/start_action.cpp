#include "stdafx.h"
#include "start_action.h"

int start_action::run(acl::string& out)
{
	start_req_t req;
	start_res_t res;
	return forward<start_req_t, start_res_t>(req, res, out);
}
