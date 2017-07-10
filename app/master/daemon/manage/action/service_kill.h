/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Thu 08 Sep 2016 04:24:52 PM CST
 */

#pragma once

struct kill_req_t;
struct kill_res_t;
struct kill_res_data_t;

class service_kill
{
public:
	service_kill(void) {}
	~service_kill(void) {}

	bool run(const kill_req_t& req, kill_res_t& res);

private:
	bool kill_one(const char* path, kill_res_data_t& data);
};
