/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Wed 14 Jun 2017 05:29:20 PM CST
 */

#pragma once

#include "http_server.h"

class manager : public acl::singleton<manager>
{
public:
	manager(void);
	~manager(void) {}

	bool init(const char* addr);
	ACL_EVENT* get_event(void) const;

private:
	acl::string     addr_;
	acl::aio_handle aio_;
	http_server     server_;
};
