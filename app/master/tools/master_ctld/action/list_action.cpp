#include "stdafx.h"
#include "list_action.h"

int list_action::run(acl::string& out)
{
	list_req_t req;
	list_res_t res;
	return forward<list_req_t, list_res_t>(req, res, out);
}
