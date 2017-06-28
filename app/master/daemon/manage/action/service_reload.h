/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Tue 27 Jun 2017 10:38:31 AM CST
 */

#pragma once

struct reload_req_t;
struct reload_res_t;
struct reload_res_data_t;

class service_reload
{
public:
	service_reload(void) {}
	~service_reload(void) {}

	bool run(const reload_req_t& req, reload_res_t& res);

private:
	bool reload_one(const char* path, reload_res_data_t& data);
};
