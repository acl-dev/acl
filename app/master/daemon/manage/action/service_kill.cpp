#include "stdafx.h"
#include "master/master_api.h"
#include "manage/http_client.h"
#include "service_kill.h"

#define CMD	"kill"

bool service_kill::kill_one(const char* path, kill_res_data_t& data)
{
	if (acl_master_kill(path) < 0) {
		data.status = 404;
		return false;
	}

	data.status = 200;
	return true;
}

bool service_kill::run(acl::json& json)
{
	kill_req_t req;
	kill_res_t res;

	if (deserialize<kill_req_t>(json, req) == false) {
		res.status = 400;
		res.msg    = "invalid json";
		client_.reply<kill_res_t>(res.status, CMD, res);
		return false;
	}

	return handle(req, res);
}

bool service_kill::handle(const kill_req_t& req, kill_res_t& res)
{
	size_t n = 0;

	for (std::vector<kill_req_data_t>::const_iterator
		cit = req.data.begin(); cit != req.data.end(); ++cit) {

		kill_res_data_t data;
		data.path = (*cit).path;

		if (kill_one((*cit).path.c_str(), data))
			n++;
		res.data.push_back(data);
	}

	if (n == req.data.size()) {
		res.status = 200;
		res.msg    = "ok";
	} else {
		res.status = 500;
		res.msg    = "error";
		logger_error("not all services were killed!, n=%d, %d",
			(int) n, (int) req.data.size());
	}

	client_.reply<kill_res_t>(res.status, CMD, res);
	client_.on_finish();

	return true;
}
