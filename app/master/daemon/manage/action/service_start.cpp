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
#include "service_start.h"

bool service_start::run(acl::json& json)
{
	start_req_t req;
	start_res_t res;

	if (deserialize<start_req_t>(json, req) == false) {
		res.status = 400;
		res.msg    = "invalid json";
		client_.reply<start_res_t>(res.status, res);
		return false;
	}

	return handle(req, res);
}

bool service_start::handle(const start_req_t& req, start_res_t& res)
{
	start_res_data_t data;
	const ACL_MASTER_SERV* serv;
	size_t  n = 0;

	for (std::vector<start_req_data_t>::const_iterator
		cit = req.data.begin(); cit != req.data.end(); ++cit) {

		const char* path = (*cit).path.c_str();
		if ((serv = acl_master_start(path)) == NULL) {
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
		logger_error("not all service have been started!, n=%d, %d",
			(int) n, (int) req.data.size());
	}

	client_.reply<start_res_t>(res.status, res);
	client_.on_finish();

	return true;
}
