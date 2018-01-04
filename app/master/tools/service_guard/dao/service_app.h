/**
 * Copyright (C) 2015-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Thu 04 Jan 2018 01:45:31 PM CST
 */

#pragma once

class service_app
{
public:
	service_app(acl::redis_client_cluster& redis) : redis_(redis) {}
	~service_app(void) {}

	bool save(const char* ip, const service_list_res_t& res);

private:
	acl::redis_client_cluster& redis_;

	bool save_one(const char* ip, const service_info_t& info);
};
