#include "stdafx.h"
#include "master/master_params.h"
#include "master/master_api.h"
#include "manage/http_client.h"
#include "type_defs.h"
#include "service_start.h"

#define CMD	"start"

service_start::service_start(http_client& client)
: client_(client)
, proc_count_(0)
, proc_signaled_(0)
, servers_finished_(0)
{
	res_.status = 200;
	timeout_ = (long long) acl_var_master_start_timeo * 1000;
}

bool service_start::run(acl::json& json)
{
	start_req_t req;

	if (deserialize<start_req_t>(json, req) == false) {
		start_res_t res;
		res.status = 400;
		res.msg    = "invalid json";
		client_.reply<start_res_t>(res.status, CMD, res);

		delete this;
		return false;
	}

	return handle(req);
}

bool service_start::handle(const start_req_t& req)
{
	bool waiting;

	if (req.timeout > 0) {
		timeout_ = req.timeout * 1000;
		waiting  = true;
	} else {
		timeout_ = 0;
		waiting  = false;
	}

	if (waiting)
		acl_event_request_timer(acl_var_master_global_event,
			service_start_timer, this, timeout_, 0);

	for (std::vector<start_req_data_t>::const_iterator
		cit = req.data.begin(); cit != req.data.end(); ++cit) {

		start_res_data_t data;

		if (start_one((*cit).path.c_str(), data, waiting, (*cit).ext)) {

			proc_count_    += data.proc_count;
			proc_signaled_ += data.proc_signaled;
			servers_[data.path] = data;
			if (!waiting) {
				data.status = 200;
				res_.data.push_back(data);
			}
		} else
			res_.data.push_back(data);
	}

	if (!waiting)
		start_finish();

	return true;
}

bool service_start::start_one(const char* path, start_res_data_t& data,
	bool waiting, const char* ext)
{
	data.status        = STATUS_TIMEOUT;
	data.path          = path;
	data.proc_count    = 0;
	data.proc_signaled = 0;
	data.proc_ok       = 0;
	data.proc_err      = 0;

	const ACL_MASTER_SERV* serv = acl_master_start(path,
			&data.proc_count, &data.proc_signaled,
			waiting ? service_start_callback : NULL,
			waiting ? this : NULL, ext);
	if (serv == NULL) {
		data.status = 500;
		data.proc_err++;
		return false;
	} else {
		data.status = timeout_ > 0 ? STATUS_TIMEOUT : 200;
		data.name   = serv->name;
		return true;
	}
}

void service_start::service_start_callback(ACL_MASTER_PROC* proc,
	int status, void *ctx)
{
	service_start *service = (service_start *) ctx;
	service->start_callback(proc, status);
}

void service_start::service_start_timer(int, ACL_EVENT*, void* ctx)
{
	service_start* service = (service_start *) ctx;
	service->timeout_callback();
}

void service_start::start_callback(ACL_MASTER_PROC* proc, int status)
{
	std::map<acl::string, start_res_data_t>::iterator it =
		servers_.find(proc->serv->conf);
	if (it == servers_.end()) {
		logger_error("not found, path=%s", proc->serv->conf);
		return;
	}

	if (status == ACL_MASTER_STAT_START_OK)
		it->second.proc_ok++;
	else {
		res_.status = 500;
		res_.msg    = "some services start failed";
		it->second.proc_err++;
	}

	if (it->second.proc_ok + it->second.proc_err < it->second.proc_count)
		return;

	if (it->second.proc_err > 0)
		it->second.status = 500;
	else
		it->second.status = 200;

	res_.data.push_back(it->second);

	if (++servers_finished_ == servers_.size())
		start_finish();
}

void service_start::timeout_callback(void)
{
	logger("start timeout reached, timeout=%lld ms", timeout_ / 1000);

	for (std::map<acl::string, start_res_data_t>::iterator
		it = servers_.begin(); it != servers_.end(); ++it) {

		if (it->second.status == STATUS_TIMEOUT)
			res_.data.push_back(it->second);
	}

	start_finish();
}

void service_start::start_finish(void)
{
	acl_event_cancel_timer(acl_var_master_global_event,
		service_start_timer, this);

	for (std::map<acl::string, start_res_data_t>::iterator
		it = servers_.begin(); it != servers_.end(); ++it) {

		acl_master_callback_clean(it->first.c_str());
	}

	client_.reply<start_res_t>(res_.status, CMD, res_);
	client_.on_finish();

	delete this;
}
