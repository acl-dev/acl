#include "stdafx.h"
#include <signal.h>
#include "master/master_params.h"
#include "master/master_api.h"
#include "manage/http_client.h"
#include "type_defs.h"
#include "service_signal.h"

#define	CMD	"signal"

service_signal::service_signal(http_client& client)
: client_(client)
, proc_count_(0)
, proc_signaled_(0)
{
	res_.status = 200;
}

bool service_signal::run(acl::json& json)
{
	signal_req_t req;

	if (!deserialize<signal_req_t>(json, req)) {
		signal_res_t res;
		res.status = 400;
		res.msg    = "invalid json";
		client_.reply<signal_res_t>(res.status, CMD, res);

		delete this;
		return false;
	}

	return handle(req);
}

bool service_signal::handle(const signal_req_t& req)
{
	for (std::vector<signal_req_data_t>::const_iterator
		cit = req.data.begin(); cit != req.data.end(); ++cit) {

		signal_res_data_t data;

		if (signal_one((*cit).path.c_str(), req.signum, data)) {
			proc_count_    += data.proc_count;
			proc_signaled_ += data.proc_signaled;
			servers_[data.path] = data;
			data.status = 200;
			res_.data.push_back(data);
		}
	}

	client_.reply<signal_res_t>(res_.status, CMD, res_);
	client_.on_finish();

	delete this;
	return true;
}

bool service_signal::signal_one(const char* path, int signum,
	signal_res_data_t& data)
{
	data.status        = 200;
	data.path          = path;
	data.proc_count    = 0;
	data.proc_signaled = 0;
	data.proc_ok       = 0;
	data.proc_err      = 0;

	int ret = acl_master_signal(path, signum, &data.proc_count,
			&data.proc_signaled, NULL, NULL);
	if (ret < 0) {
		data.status = 404;
		data.proc_err++;
		return false;
	}

	return true;
}
