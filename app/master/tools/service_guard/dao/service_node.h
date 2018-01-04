/**
 * Copyright (C) 2015-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Thu 04 Jan 2018 01:32:34 PM CST
 */

#pragma once

class service_node
{
public:
	service_node(acl::redis_client_cluster& redis) : redis_(redis) {}
	~service_node(void) {}

	bool save(const char* ip, const service_list_res_t& res);

private:
	acl::redis_client_cluster& redis_;
};
