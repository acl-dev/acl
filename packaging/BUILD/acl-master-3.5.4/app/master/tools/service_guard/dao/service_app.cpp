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
#define APP_DATE	"date"
#define APP_VER		"version"
#define APP_FDS		"fds"
#define APP_MEM		"mem"
#define APP_CPU		"cpu"
#define APP_IO		"io"

bool service_app::save_one(const char* ip, const service_info_t& info)
{
	std::map<acl::string, acl::string> attrs;

	attrs[APP_CONF]   = info.conf;
	attrs[APP_NAME]   = info.name;
	attrs[APP_STATUS] = acl::string::parse_int(info.status);
	attrs[APP_VER]    = info.version;
	attrs[APP_FDS]    = acl::string::parse_int(info.fds);
	attrs[APP_MEM]    = acl::string::parse_int64((long long) info.mem);

	acl::rfc822 rfc;
	char buf[128];

	rfc.mkdate_cst(info.start, buf, sizeof(buf));
	attrs[APP_START]  = buf;

	time_t now = time(NULL);
	rfc.mkdate_cst(now, buf, sizeof(buf));
	attrs[APP_DATE]   = buf;

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
