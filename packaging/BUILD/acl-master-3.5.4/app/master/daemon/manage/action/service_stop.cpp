#include "stdafx.h"
#include "master/master_api.h"
#include "manage/http_client.h"
#include "service_stop.h"

#define CMD	"stop"

bool service_stop::stop_one(const char* path, stop_res_data_t& data)
{
	if (acl_master_stop(path) < 0) {
		data.status = 404;
		return false;
	}

	data.status = 200;
	return true;
}

bool service_stop::run(acl::json& json)
{
	stop_req_t req;
	stop_res_t res;

	if (deserialize<stop_req_t>(json, req) == false) {
		res.status = 400;
		res.msg    = "invalid json";
		client_.reply<stop_res_t>(res.status, CMD, res);
		return false;
	}

	return handle(req, res);
}

bool service_stop::handle(const stop_req_t& req, stop_res_t& res)
{
	size_t n = 0;

	for (std::vector<stop_req_data_t>::const_iterator
		cit = req.data.begin(); cit != req.data.end(); ++cit) {

		stop_res_data_t data;
		data.path = (*cit).path;

		if (stop_one((*cit).path.c_str(), data))
			n++;
		res.data.push_back(data);
	}

	if (n == req.data.size()) {
		res.status = 200;
		res.msg    = "ok";
	} else {
		res.status = 500;
		res.msg    = "error";
		logger_error("not all services were started!, n=%d, %d",
			(int) n, (int) req.data.size());
	}

	client_.reply<stop_res_t>(res.status, CMD, res);
	client_.on_finish();

	return true;
}
