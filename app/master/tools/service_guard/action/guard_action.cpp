/**
 * Copyright (C) 2015-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Thu 04 Jan 2018 11:21:39 AM CST
 */

#include "stdafx.h"
#include "dao/service_node.h"
#include "dao/service_app.h"
#include "guard_action.h"

guard_action::guard_action(const char* ip, const char* data)
: ip_(ip)
, data_(data)
{
}

guard_action::~guard_action(void)
{
}

void* guard_action::run(void)
{
	do_run();
	delete this;
	return NULL;
}

bool guard_action::do_run(void)
{
	acl::json json(data_);
	if (!json.finish()) {
		logger_error("invalid data=|%s|", data_.c_str());
		return false;
	}

	service_list_res_t res;
	if (deserialize<service_list_res_t>(json, res) == false) {
		return false;
	}

	service_node node(var_redis);
	if (node.save(ip_, res) == false) {
		logger_error("save error, ip=%s", ip_.c_str());
		return false;
	}

	service_app app(var_redis);
	if (app.save(ip_, res) == false) {
		logger_error("save app info error, ip=%s", ip_.c_str());
		return false;
	}

	return true;
}
