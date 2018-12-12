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

template<typename TReq, typename TReqData>
bool commands_action::enabled(acl::json& in, acl::string& out)
{
	TReq req;
	if (deserialize<TReq>(in, req) == false)
	{
		logger_error("invalid json=%s", in.to_string().c_str());
		res_t res;
		res.status = 400;
		res.msg    = "invalid json";
		serialize<res_t>(res, out);
		return false;
	}

	for (typename std::vector<TReqData>::const_iterator cit
		= req.data.begin(); cit != req.data.end(); ++cit)
	{
		if (conf_ == (*cit).path)
		{
			logger_error("disable operation on %s", conf_.c_str());
			res_t res;
			res.status = 400;
			res.msg    = "disable!";
			serialize<res_t>(res, out);
			return false;
		}
	}

	return true;
}

int commands_action::run(acl::string& out)
{
	acl::json* json = get_json();
	if (json == NULL)
	{
		logger_error("json null");
		return 400;
	}

	if (cmd_ == "start")
	{
		if (!enabled<start_req_t, start_req_data_t>(*json, out))
			return 400;
		return http_forward<start_req_t, start_res_t>(addr_, *json, out);
	}
	else if (cmd_ == "stop")
	{
		if (!enabled<stop_req_t, stop_req_data_t>(*json, out))
			return 400;
		return http_forward<stop_req_t, stop_res_t>(addr_, *json, out);
	}
	else if (cmd_ == "kill")
	{
		if (!enabled<kill_req_t, kill_req_data_t>(*json, out))
			return 400;
		return http_forward<kill_req_t, kill_res_t>(addr_, *json, out);
	}
	else if (cmd_ == "reload")
	{
		if (!enabled<reload_req_t, reload_req_data_t>(*json, out))
			return 400;
		return http_forward<reload_req_t, reload_res_t>(addr_, *json, out);
	}
	else if (cmd_ == "restart")
	{
		if (!enabled<restart_req_t, restart_req_data_t>(*json, out))
			return 400;
		return http_forward<restart_req_t, restart_res_t>(addr_, *json, out);
	}

	// the other commands maybe be safety for master_ctld
	else
		return http_forward(addr_, cmd_, json->to_string(), out);
}
