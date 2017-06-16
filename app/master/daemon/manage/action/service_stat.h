/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Fri 16 Jun 2017 01:53:06 PM CST
 */

#pragma once

struct stat_req_t;
struct stat_res_t;
struct serv_info_t;

class service_stat
{
public:
	service_stat(void) {}
	~service_stat(void) {}

	bool run(const stat_req_t& req, stat_res_t& res);

private:
	bool stat_one(const char* name, serv_info_t& info);
};
