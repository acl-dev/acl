/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Fri 16 Jun 2017 10:48:20 AM CST
 */

#include "stdafx.h"
#include "master/master_api.h"
#include "service_list.h"

void service_list::add_one(list_res_t& res, const ACL_MASTER_SERV* serv)
{
	serv_info_t info;

	info.status          = 0;
	info.name            = serv->name;
	info.path            = serv->path;
	info.proc_max        = serv->max_proc;
	info.proc_prefork    = serv->prefork_proc;
	info.proc_total      = serv->total_proc;
	info.proc_avail      = serv->avail_proc;
	info.throttle_delay  = serv->throttle_delay;
	info.listen_fd_count = serv->listen_fd_count;

	if (serv->owner && *serv->owner)
		info.owner             = serv->owner;
	if (serv->notify_addr && *serv->notify_addr)
		info.notify_addr       = serv->notify_addr;
	if (serv->notify_recipients && *serv->notify_recipients)
		info.notify_recipients = serv->notify_recipients;

	ACL_ITER iter;
	acl_foreach(iter, serv->children_env)
	{
		ACL_MASTER_NV* v = (ACL_MASTER_NV *) iter.data;
		info.env[v->name] = v->value;
	}

	res.data.push_back(info);
}

bool service_list::run(const list_req_t&, list_res_t& res)
{
	ACL_MASTER_SERV *serv;

	for (serv = acl_var_master_head; serv != 0; serv = serv->next)
		add_one(res, serv);

	res.status = 200;
	res.msg    = "ok";

	return true;
}
