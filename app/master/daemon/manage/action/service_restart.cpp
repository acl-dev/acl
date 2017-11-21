/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Fri 16 Jun 2017 02:00:01 PM CST
 */

#include "stdafx.h"
#include "master/master_api.h"
#include "manage/http_client.h"
#include "service_restart.h"

bool service_restart::run(acl::json& json)
{
	restart_req_t req;
	restart_res_t res;

	if (deserialize<restart_req_t>(json, req) == false) {
		res.status = 400;
		res.msg    = "invalid json";
		client_.reply<restart_res_t>(res.status, res);
		return false;
	}

	return handle(req, res);
}

bool service_restart::handle(const restart_req_t& req, restart_res_t& res)
{
	restart_res_data_t data;
	const ACL_MASTER_SERV* serv;
	size_t  n = 0;

	for (std::vector<restart_req_data_t>::const_iterator
		cit = req.data.begin(); cit != req.data.end(); ++cit) {

		const char* path = (*cit).path.c_str();
		serv = acl_master_restart(path, NULL, NULL, NULL, NULL,
			(*cit).ext.c_str());
		if (serv == NULL) {
			data.status = 500;
			data.path   = path;
		} else {
			data.status = 200;
			data.name   = serv->name;
			data.path   = serv->path;
			n++;
		}

		res.data.push_back(data);
	}

	if (n == req.data.size()) {
		res.status = 200;
		res.msg    = "ok";
	} else {
		res.status = 500;
		res.msg    = "error";
		logger_error("not all service have been restarted!, n=%d, %d",
			(int) n, (int) req.data.size());
	}

	client_.reply<restart_res_t>(res.status, res);
	client_.on_finish();

	return true;
}
