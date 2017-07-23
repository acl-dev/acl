#include "stdafx.h"
#include "sysload.h"

double sysload::get_load(acl::string* out)
{
	acl::string buf;
	if (acl::ifstream::load("/proc/loadavg", &buf) == false)
	{
		logger_error("can't system load: %s", acl::last_serror());
		return -1;
	}

	std::vector<acl::string>& tokens = buf.split2(" \t");
	if (out)
		*out = tokens[0].c_str();

	return atof(tokens[0].c_str());
}
