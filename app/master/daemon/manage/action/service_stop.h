/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Fri 16 Jun 2017 01:56:53 PM CST
 */

#pragma once

struct stop_req_t;
struct stop_res_t;
struct stop_res_data_t;

class service_stop
{
public:
	service_stop(void) {}
	~service_stop(void) {}

	bool run(const stop_req_t& req, stop_res_t& res);

private:
	bool stop_one(const char* name, stop_res_data_t& data);
};
