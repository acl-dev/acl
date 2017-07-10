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
#include "master/master_api.h"
#include "service_reload.h"

bool service_reload::reload_one(const char* path, reload_res_data_t& data)
{
	data.proc_count = 0;
	data.proc_signaled = 0;

	if (acl_master_reload(path, &data.proc_count, &data.proc_signaled) < 0)
	{
		data.status = 404;
		return false;
	}

	data.status = 200;
	return true;
}

bool service_reload::run(const reload_req_t& req, reload_res_t& res)
{
	size_t n = 0;

	for (std::vector<reload_req_data_t>::const_iterator
		cit = req.data.begin(); cit != req.data.end(); ++cit)
	{
		reload_res_data_t data;
		data.path = (*cit).path;
		if (reload_one((*cit).path.c_str(), data))
			n++;
		res.data.push_back(data);
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
		logger_error("not all services were reloaded!, n=%d, %d",
			(int) n, (int) req.data.size());
	}

	return true;
}
