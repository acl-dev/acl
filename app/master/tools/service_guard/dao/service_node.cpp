/**
 * Copyright (C) 2015-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Thu 04 Jan 2018 01:33:10 PM CST
 */

#include "stdafx.h"
#include "service_node.h"

bool service_node::save(const char* ip, const service_list_res_t& res)
{
	std::map<acl::string, double> members;
	time_t now = time(NULL);

	for (std::vector<service_info_t>::const_iterator cit = res.data.begin();
		cit != res.data.end(); ++cit) {
		members[(*cit).conf] = now;
	}

	acl::redis cmd(&redis_);
	if (cmd.zadd(ip, members) < 0) {
		logger_error("hmset error=%s, ip=%s", cmd.result_error(), ip);
		return false;
	}
	return true;
}
