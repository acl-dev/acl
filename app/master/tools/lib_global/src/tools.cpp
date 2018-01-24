#include "stdafx.h"
#include "tools.h"

bool tools::get_line(ACL_VSTREAM* fp, acl::string& out)
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
		logger_error("popen error=%s, cmd=%s",
			acl::last_serror(), path);
		acl_argv_free(args);
		return false;
	}

	bool ret = get_line(fp, out);
	acl_vstream_pclose(fp);
	acl_argv_free(args);

	return ret;
}

int tools::get_fds(pid_t pid)
{
	acl::string buf;
	buf.format("/proc/%d/fd", (int) pid);

	ACL_ARGV* args = acl_argv_alloc(1);
	acl_argv_add(args, "ls", buf.c_str(), "|", "wc", "-l", NULL);

	ACL_VSTREAM* fp = acl_vstream_popen(O_RDWR,
		ACL_VSTREAM_POPEN_ARGV, args->argv, ACL_VSTREAM_POPEN_END);
	if (fp == NULL)
	{
		logger_error("popen error=%s, cmd=ls /proc/%d/fd|wc -l",
			acl::last_serror(), (int) pid);
		return -1;
	}

	char line[1024];
	int ret = acl_vstream_gets_nonl(fp, line, sizeof(line));
	if (ret == ACL_VSTREAM_EOF)
	{
		logger_error("gets line error=%s, cmd=ls /proc/%d/fd|wc -l",
				acl::last_serror(), (int) pid);
		acl_argv_free(args);
		acl_vstream_pclose(fp);
		return -1;
	}

	ret = atoi(line);
	if (ret < 0)
		logger_error("invalid fdcount=%d, cmd=ls /proc/%d/fd|wc -l",
			ret, (int) pid);
	acl_argv_free(args);
	acl_vstream_pclose(fp);

	return ret;
}
