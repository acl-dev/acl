/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Wed 14 Jun 2017 05:30:59 PM CST
 */

#include "stdafx.h"
#include "manager.h"

manager::manager(void)
	: aio_(acl::ENGINE_KERNEL)
	, server_(aio_)
{
}

bool manager::init(const char* addr)
{
	return server_.open(addr);
}

ACL_EVENT* manager::get_event(void) const
{
	ACL_AIO*   aio = aio_.get_handle();
	ACL_EVENT* event = acl_aio_event(aio);
	return event;
}
