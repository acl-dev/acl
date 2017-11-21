/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Fri 16 Jun 2017 10:46:35 AM CST
 */

#pragma once

struct master_config_req_t;
struct master_config_res_t;
class http_client;

class service_master_config
{
public:
	service_master_config(http_client& client) : client_(client) {}
	~service_master_config(void) {}

	bool run(acl::json& json);

private:
	http_client& client_;
};
