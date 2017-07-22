#include "stdafx.h"
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
	if (cmd_ == "list")
	{
		list_req_t req;
		list_res_t res;
		return forward<list_req_t, list_res_t>(req, res, out);
	}
	else if (cmd_ == "stat")
	{
		stat_req_t req;
		stat_res_t res;
		return forward<stat_req_t, stat_res_t>(req, res, out);
	}
	else if (cmd_ == "start")
	{
		start_req_t req;
		start_res_t res;
		return forward<start_req_t, start_res_t>(req, res, out);
	}
	else if (cmd_ == "stop")
	{
		stop_req_t req;
		stop_res_t res;
		return forward<stop_req_t, stop_res_t>(req, res, out);
	}
	else if (cmd_ == "kill")
	{
		kill_req_t req;
		kill_res_t res;
		return forward<kill_req_t, kill_res_t>(req, res, out);
	}
	else if (cmd_ == "reload")
	{
		reload_req_t req;
		reload_res_t res;
		return forward<reload_req_t, reload_res_t>(req, res, out);
	}
	else if (cmd_ == "restart")
	{
		restart_req_t req;
		restart_res_t res;
		return forward<restart_req_t, restart_res_t>(req, res, out);
	}
	else
	{
		logger_error("unknown cmd=%s", cmd_.c_str());
		return false;
	}
}
