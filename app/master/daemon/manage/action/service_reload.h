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
#include "master/master.h"

class  http_client;
struct reload_req_t;
struct reload_res_t;
struct reload_res_data_t;

class service_reload
{
public:
	service_reload(http_client& client, reload_res_t& res)
	: client_(client)
	, res_(res)
	, timeout_(5000000)
	, proc_count_(0)
	, proc_signaled_(0)
	, servers_finished_(0)
	{
	}
	~service_reload(void) {}

	bool run(const reload_req_t& req);

private:
	http_client&  client_;
	reload_res_t& res_;
	long long     timeout_;
	int    proc_count_;
	int    proc_signaled_;
	size_t servers_finished_;
	std::map<acl::string, reload_res_data_t> servers_;

	bool reload_one(const char* path, reload_res_data_t& data);
	static void service_reload_timer(int, ACL_EVENT* event, void* ctx);
	static void service_reload_callback(ACL_MASTER_PROC* proc, int sig,
			int status, void* ctx);
	void check_all(void);
	void clean_all(void);
	void on_timeout(void);
};
