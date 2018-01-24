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

struct service_base
{
	int  status;
	acl::string cmd;
};

struct service_info_t
{
	int  status;
	long start;
	acl::string name;
	acl::string conf;
	// Gson@optional
	acl::string path;
	// Gson@optional
	acl::string version;
};

struct service_list_res_t : service_base
{
	std::vector<service_info_t> data;
};
