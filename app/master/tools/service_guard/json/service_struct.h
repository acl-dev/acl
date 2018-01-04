/**
 * Copyright (C) 2015-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Wed 03 Jan 2018 05:51:29 PM CST
 */

#pragma once

struct service_info_t
{
	int  status;
	long start;
	acl::string name;
	acl::string conf;
};

struct service_list_res_t
{
	int  status;
	acl::string msg;
	std::vector<service_info_t> data;
};
