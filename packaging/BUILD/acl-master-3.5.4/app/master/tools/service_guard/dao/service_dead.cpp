#include "stdafx.h"
#include "service_dead.h"

#define DEAD		"dead"
#define PATH		"path"
#define VERSION		"version"
#define PID		"pid"
#define RCPT		"rcpt"
#define INFO		"info"
#define DATE		"date"

bool service_dead::save(const char* ip, const service_dead_res_t& res)
{
	std::map<acl::string, acl::string> attrs;

	attrs[PATH]    = res.path;
	attrs[PID]     = acl::string::parse_int(res.pid);
	attrs[VERSION] = res.version;
	attrs[RCPT]    = res.rcpt;
	attrs[INFO]    = res.info;

	char buf[128];
	acl::rfc822 rfc;
	time_t now = time(NULL);
	rfc.mkdate_cst(now, buf, sizeof(buf));
	attrs[DATE] = buf;

	acl::string key;
	key.format("%s|%s|%s", DEAD, ip, res.path.c_str());
	acl::redis cmd(&redis_);
	if (cmd.hmset(key, attrs) == false) {
		logger_error("save error=%s, key=%s, ip=%s, path=%s",
			cmd.result_error(), key.c_str(), ip, res.path.c_str());
		return false;
	}

	return true;
}
