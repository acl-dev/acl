#include "stdafx.h"
#include "http_forward.h"
#include "commands_action.h"

commands_action::commands_action(const char* addr,
	acl::HttpServletRequest& req, acl::HttpServletResponse& res,
	const char* cmd)
: base_action(addr, req, res)
, cmd_(cmd)
{
	cmd_.lower();
}

int commands_action::run(acl::string& out)
{
	acl::json* json = get_json();
	if (json == NULL)
	{
		logger_error("json null");
		return 400;
	}

	if (cmd_ == "list")
		return http_forward<list_req_t, list_res_t>(addr_, *json, out);
	else if (cmd_ == "stat")
		return http_forward<stat_req_t, stat_res_t>(addr_, *json, out);
	else if (cmd_ == "start")
		return http_forward<start_req_t, start_res_t>(addr_, *json, out);
	else if (cmd_ == "stop")
		return http_forward<stop_req_t, stop_res_t>(addr_, *json, out);
	else if (cmd_ == "kill")
		return http_forward<kill_req_t, kill_res_t>(addr_, *json, out);
	else if (cmd_ == "reload")
		return http_forward<reload_req_t, reload_res_t>(addr_, *json, out);
	else if (cmd_ == "restart")
		return http_forward<restart_req_t, restart_res_t>(addr_, *json, out);
	else
	{
		logger_error("unknown cmd=%s", cmd_.c_str());
		return 400;
	}
}
