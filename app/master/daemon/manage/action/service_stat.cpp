/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Fri 16 Jun 2017 01:54:45 PM CST
 */

#include "stdafx.h"
#include "master/master_api.h"
#include "service_stat.h"

bool service_stat::stat_one(const char* name, serv_info_t& info)
{
	ACL_MASTER_SERV *serv = acl_master_lookup(name);

	if (serv == NULL)
	{
		info.status = 404;
		return false;
	}

	info.name            = serv->name;
	info.path            = serv->path;
	info.proc_max        = serv->max_proc;
	info.proc_prefork    = serv->prefork_proc;
	info.proc_total      = serv->total_proc;
	info.proc_avail      = serv->avail_proc;
	info.throttle_delay  = serv->throttle_delay;
	info.listen_fd_count = serv->listen_fd_count;

	if (serv->owner && *serv->owner)
		info.owner             = serv->owner;
	if (serv->notify_addr && *serv->notify_addr)
		info.notify_addr       = serv->notify_addr;
	if (serv->notify_recipients && *serv->notify_recipients)
		info.notify_recipients = serv->notify_recipients;

	ACL_ITER iter;
	acl_foreach(iter, serv->children_env)
	{
		ACL_MASTER_NV* v = (ACL_MASTER_NV *) iter.data;
		info.env[v->name] = v->value;
	}

	info.status = 200;
	return true;
}

bool service_stat::run(const stat_req_t& req, stat_res_t& res)
{
	size_t n = 0;

	for (std::vector<acl::string>::const_iterator cit = req.data.begin();
		cit != req.data.end(); ++cit)
	{
		serv_info_t info;
		if (stat_one((*cit).c_str(), info))
			n++;
		res.data.push_back(info);
	}

	if (n == req.data.size())
	{
		res.status = 200;
		res.msg    = "ok";
	}
	else
	{
		res.status = 500;
		res.msg    = "error";
		logger_error("not all service have been started!, n=%d, %d",
			(int) n, (int) req.data.size());
	}

	return true;
}
