/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Tue 27 Jun 2017 10:43:03 AM CST
 */

#include "stdafx.h"
#include <signal.h>
#include "master/master_api.h"
#include "manage/http_client.h"
#include "service_reload.h"

#define STATUS_TIMEOUT	503

bool service_reload::reload_one(const char* path, reload_res_data_t& data)
{
	data.status        = STATUS_TIMEOUT;
	data.path          = path;
	data.proc_count    = 0;
	data.proc_signaled = 0;
	data.proc_ok       = 0;
	data.proc_err      = 0;

	int ret = acl_master_reload(path, &data.proc_count,
		&data.proc_signaled, service_reload_callback, this);
	if (ret < 0)
	{
		data.status = 404;
		return false;
	}

	return true;
}

bool service_reload::run(const reload_req_t& req)
{
	size_t n = 0;

	if (req.timeout > 0)
		timeout_ = req.timeout * 1000;

	logger(">>>>timeout_: %lld, %lld<<<", timeout_, req.timeout);

	acl_event_request_timer(acl_var_master_global_event,
		service_reload_timer, this, timeout_, 0);

	for (std::vector<reload_req_data_t>::const_iterator
		cit = req.data.begin(); cit != req.data.end(); ++cit)
	{
		reload_res_data_t data;
		if (reload_one((*cit).path.c_str(), data))
		{
			proc_count_    += data.proc_count;
			proc_signaled_ += data.proc_signaled;
			servers_[data.path] = data;
			n++;
		}
		else
			res_.data.push_back(data);
	}

	return true;
}

void service_reload::service_reload_timer(int, ACL_EVENT*, void* ctx)
{
	service_reload* reload = (service_reload *) ctx;
	reload->on_timeout();
}

void service_reload::service_reload_callback(ACL_MASTER_PROC* proc, int sig,
	int status, void* ctx)
{
	service_reload* reload = (service_reload *) ctx;
	if (sig != SIGHUP)
	{
		logger_error("not SIGHUP, invalid signum=%d", sig);
		return;
	}

	std::map<acl::string, reload_res_data_t>::iterator it =
		reload->servers_.find(proc->serv->conf);
	if (it == reload->servers_.end())
	{
		logger_error("not found, path=%s", proc->serv->conf);
		return;
	}

	if (status == ACL_MASTER_STAT_SIGHUP_OK)
		it->second.proc_ok++;
	else
	{
		reload->res_.status = 500;
		reload->res_.msg    = "some services reload failed";
		it->second.proc_err++;
	}

	if (it->second.proc_ok + it->second.proc_err == it->second.proc_count)
	{
		if (it->second.proc_err > 0)
			it->second.status = 500;
		else
			it->second.status = 200;
		reload->res_.data.push_back(it->second);
		reload->servers_finished_++;
		reload->check_all();
	}
}

void service_reload::check_all(void)
{
	if (servers_finished_ == servers_.size())
	{
		clean_all();
		client_.on_reload_finish();
	}
}

void service_reload::clean_all(void)
{
	acl_event_cancel_timer(acl_var_master_global_event,
		service_reload_timer, this);
	for (std::map<acl::string, reload_res_data_t>::iterator
		it = servers_.begin(); it != servers_.end(); ++it)
	{
		acl_master_reload_clean(it->first.c_str());
	}
}

void service_reload::on_timeout(void)
{
	clean_all();

	logger("reload timeout reached, timeout=%lld ms", timeout_ / 1000);

	for (std::map<acl::string, reload_res_data_t>::iterator
		it = servers_.begin(); it != servers_.end(); ++it)
	{
		if (it->second.status == STATUS_TIMEOUT)
			res_.data.push_back(it->second);
	}

	client_.on_reload_finish();
}
