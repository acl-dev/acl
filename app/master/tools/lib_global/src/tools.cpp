#include "stdafx.h"
#include "tools.h"

bool tools::get_version(ACL_VSTREAM* fp, acl::string& out)
{
	char buf[8192];

	int ret = acl_vstream_gets_nonl(fp, buf, sizeof(buf));
	if (ret == ACL_VSTREAM_EOF)
	{
		logger_error("read error %s", acl::last_serror());
		return false;
	}

	out = buf;
	return true;
}

bool tools::get_version(const char* path, acl::string& out)
{
	out = "unknown";

	ACL_ARGV* args = acl_argv_alloc(1);
	acl_argv_add(args, path, "-v", NULL);

	ACL_VSTREAM* fp = acl_vstream_popen(O_RDWR,
		ACL_VSTREAM_POPEN_ARGV, args->argv, ACL_VSTREAM_POPEN_END);
	if (fp == NULL)
	{
		logger_error("popen error=%s, comd=%s",
			acl::last_serror(), path);
		return false;
	}
	else
	{
		get_version(fp, out);
		acl_vstream_pclose(fp);
	}

	acl_argv_free(args);
	return true;
}
