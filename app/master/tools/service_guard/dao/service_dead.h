/**
 * Copyright (C) 2015-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Wed 24 Jan 2018 11:22:03 PM CST
 */

#pragma once

class service_dead
{
public:
	service_dead(acl::redis_client_cluster& redis) : redis_(redis) {}
	~service_dead(void) {}

	bool save(const char* ip, const service_dead_res_t& res);

private:
	acl::redis_client_cluster& redis_;
};
