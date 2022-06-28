#include "stdafx.h"
#include "base_action.h"

base_action::base_action(const acl::string& addr,
	acl::HttpServletRequest& req, acl::HttpServletResponse& res)
: addr_(addr)
, req_(req)
, res_(res)
{
}

void base_action::set_conf(const char* path)
{
	conf_ = path;
}

acl::json* base_action::get_json(void)
{
	acl::json* json = req_.getJson();
	if (json == NULL)
	{
		const char* ctype = req_.getContentType();
		logger_error("getJson null, content-type: %s",
			ctype ? ctype : "unknown");
	}

	return json;
}
