#include "stdafx.h"
#include "reload_action.h"

int reload_action::run(acl::string& out)
{
	reload_req_t req;
	reload_res_t res;
	return forward<reload_req_t, reload_res_t>(req, res, out);
}
