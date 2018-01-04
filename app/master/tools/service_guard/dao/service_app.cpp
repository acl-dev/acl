/**
 * Copyright (C) 2015-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Thu 04 Jan 2018 01:54:01 PM CST
 */

#include "stdafx.h"
#include "service_app.h"

bool service_app::save(const char* ip, const service_list_res_t& res)
{
	int  n = 0;
	for (std::vector<service_info_t>::const_iterator cit = res.data.begin();
		cit != res.data.end(); ++cit) {

		if (save_one(ip, *cit)) {
			n++;
		}
	}

	if (n != (int) res.data.size()) {
		logger_warn("not all been saved, ip=%s, saved=%d, left=%d",
			ip, (int) n, ((int) res.data.size() - n));
	}
	return true;
}

#define APP		"app"
#define	APP_CONF	"conf"
#define APP_NAME	"name"
#define APP_START	"start"
#define APP_STATUS	"status"

bool service_app::save_one(const char* ip, const service_info_t& info)
{
	std::map<acl::string, acl::string> attrs;

	attrs[APP_CONF]   = info.conf;
	attrs[APP_NAME]   = info.name;
	attrs[APP_START]  = acl::string::parse_int64((long long) info.start);
	attrs[APP_STATUS] = acl::string::parse_int(info.status);

	acl::string key;
	key.format("%s|%s|%s", APP, ip, info.conf.c_str());
	acl::redis cmd(&redis_);
	if (cmd.hmset(key, attrs) == false) {
		logger_error("save error=%s, key=%s, ip=%s, conf=%s",
			cmd.result_error(), key.c_str(), ip, info.conf.c_str());
		return false;
	}

	return true;
}
