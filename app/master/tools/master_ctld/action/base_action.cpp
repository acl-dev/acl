#include "stdafx.h"
#include "base_action.h"

base_action::base_action(void)
: req_(NULL)
, res_(NULL)
{
}

void base_action::prepare(const acl::string& addr,
	acl::HttpServletRequest& req, acl::HttpServletResponse& res)
{
	addr_ = addr;
	req_ = &req;
	res_ = &res;
}

acl::json* base_action::get_json(void)
{
	assert(req_ && res_);
	acl::json* json = req_->getJson();
	if (json == NULL)
	{
		const char* ctype = req_->getContentType();
		logger_error("getJson null, content-type: %s",
			ctype ? ctype : "unknown");
	}

	return json;
}
