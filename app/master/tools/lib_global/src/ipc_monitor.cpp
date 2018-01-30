/**
 * Copyright (C) 2015-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Mon 29 Jan 2018 05:14:37 PM CST
 */

#include "stdafx.h"
#include "ipc_monitor.h"

ipc_monitor::ipc_monitor(acl::tcp_ipc& ipc, int ttl, bool& service_exit)
: ipc_(ipc)
, ttl_(ttl)
, service_exit_(service_exit)
{
}

void* ipc_monitor::run(void)
{
	logger("ipc_monitor started!");

	while (!service_exit_)
	{
		sleep(1);
		check_idle();
	}

	logger("ipc_monitor stopped now!");
	return NULL;
}

void ipc_monitor::check_idle(void)
{
	acl::tcp_manager& manager = ipc_.get_manager();
	std::vector<acl::connect_pool*>& pools = manager.get_pools();
	for (std::vector<acl::connect_pool*>::iterator it = pools.begin();
		it != pools.end(); ++it)
	{
		(*it)->check_idle(ttl_);
	}
}
