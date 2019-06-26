#include "stdafx.h"
#include <signal.h>
#include "master/master_params.h"
#include "master/master_api.h"
#include "manage/http_client.h"
#include "type_defs.h"
#include "service_reload.h"

#define CMD	"reload"

service_reload::service_reload(http_client& client)
: client_(client)
, proc_count_(0)
, proc_signaled_(0)
, servers_finished_(0)
{
	res_.status = 200;
	timeout_ = (long long) acl_var_master_reload_timeo * 1000;
}

bool service_reload::run(acl::json& json)
{
	reload_req_t req;

	//logger(">>>%s<<<", json_.to_string().c_str());
	if (deserialize<reload_req_t>(json, req) == false) {
		reload_res_t res;
		res.status = 400;
		res.msg    = "invalid json";
		client_.reply<reload_res_t>(res.status, CMD, res);

		delete this;
		return false;
	}

	return handle(req);
}

bool service_reload::handle(const reload_req_t& req)
{
	bool waiting;

	if (req.timeout > 0) {
		timeout_ = req.timeout * 1000;
		waiting  = true;
	} else {
		timeout_ = 0;
		waiting  = false;
	}

	// logger(">>>>timeout_: %lld, %lld<<<", timeout_, req.timeout);

	if (waiting)
		acl_event_request_timer(acl_var_master_global_event,
			service_reload_timer, this, timeout_, 0);

	for (std::vector<reload_req_data_t>::const_iterator
		cit = req.data.begin(); cit != req.data.end(); ++cit) {

		reload_res_data_t data;

		if (reload_one((*cit).path.c_str(), data, waiting)) {
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
		reload_finish();

	return true;
}

bool service_reload::reload_one(const char* path, reload_res_data_t& data,
	bool waiting)
{
	data.status        = STATUS_TIMEOUT;
	data.path          = path;
	data.proc_count    = 0;
	data.proc_signaled = 0;
	data.proc_ok       = 0;
	data.proc_err      = 0;

	int ret = acl_master_reload(path, &data.proc_count, &data.proc_signaled,
		waiting ? service_reload_callback : NULL,
		waiting ? this : NULL);
	if (ret < 0) {
		data.status = 404;
		data.proc_err++;
		return false;
	}

	return true;
}

void service_reload::service_reload_timer(int, ACL_EVENT*, void* ctx)
{
	service_reload* reload = (service_reload *) ctx;
	reload->timeout_callback();
}

void service_reload::service_reload_callback(ACL_MASTER_PROC* proc,
	int status, void* ctx)
{
	service_reload* service = (service_reload *) ctx;
	service->reload_callback(proc, status);
}

void service_reload::reload_callback(ACL_MASTER_PROC* proc, int status)
{
	std::map<acl::string, reload_res_data_t>::iterator it =
		servers_.find(proc->serv->conf);
	if (it == servers_.end()) {
		logger_error("not found, path=%s", proc->serv->conf);
		return;
	}

	if (status == ACL_MASTER_STAT_SIGHUP_OK)
		it->second.proc_ok++;
	else {
		res_.status = 500;
		res_.msg    = "some services reload failed";
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
		reload_finish();
}

void service_reload::timeout_callback(void)
{
	logger("reload timeout reached, timeout=%lld ms", timeout_ / 1000);

	for (std::map<acl::string, reload_res_data_t>::iterator
		it = servers_.begin(); it != servers_.end(); ++it) {

		if (it->second.status == STATUS_TIMEOUT)
			res_.data.push_back(it->second);
	}

	reload_finish();
}

void service_reload::reload_finish(void)
{
	acl_event_cancel_timer(acl_var_master_global_event,
		service_reload_timer, this);

	for (std::map<acl::string, reload_res_data_t>::iterator
		it = servers_.begin(); it != servers_.end(); ++it) {

		acl_master_callback_clean(it->first.c_str());
	}

	client_.reply<reload_res_t>(res_.status, CMD, res_);
	client_.on_finish();

	delete this;
}
