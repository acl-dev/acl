/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Fri 16 Jun 2017 01:59:13 PM CST
 */

#pragma once

struct start_req_t;
struct start_res_t;
class http_client;

class service_start
{
public:
	service_start(http_client& client) : client_(client) {}
	~service_start(void) {}

	bool run(acl::json& json);

private:
	http_client& client_;

	bool handle(const start_req_t& req, start_res_t& res);
};
