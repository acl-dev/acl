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

struct ACL_MASTER_SERV;
class list_req_t;
class list_res_t;

class service_list
{
public:
	service_list(void) {}
	~service_list(void) {}

	bool run(const list_req_t& req, list_res_t& res);

private:
	void add_one(list_res_t& res, const ACL_MASTER_SERV* ser);
};
