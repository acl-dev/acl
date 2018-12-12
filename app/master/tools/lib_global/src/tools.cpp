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

long tools::get_mem(pid_t pid)
{
	acl::string path;
	path.format("/proc/%d/statm", (int) pid);
	FILE* fp = fopen(path.c_str(), "r");
	if (fp == NULL)
	{
		logger_error("open %s error=%s",
			path.c_str(), acl::last_serror());
		return -1;
	}

	long n_rss, n_resident, n_share, n_text, n_lib, n_data, n_dt;
	int n = fscanf(fp,"%lu%lu%lu%lu%lu%lu%lu", &n_rss, &n_resident,
			&n_share, &n_text, &n_lib, &n_data, &n_dt);
	fclose(fp);
	if (n != 7)
	{
		logger_error("fscanf error, n=%d", n);
		return -1;
	}

	n_rss = n_resident * 4 / 1024;
	return n_rss;
}

bool tools::exec_shell(const char* cmd, acl::string& buf)
{
	ACL_VSTREAM* fp = acl_vstream_popen(O_RDWR,
		ACL_VSTREAM_POPEN_COMMAND, cmd, ACL_VSTREAM_POPEN_END);
	if (fp == NULL)
	{
		logger_error("popen error=%s, cmd=%s", acl::last_serror(), cmd);
		return false;
	}

	char line[1024];

	while (acl_vstream_gets_nonl(fp, line, sizeof(line)) != ACL_VSTREAM_EOF)
		buf += line;

	acl_vstream_pclose(fp);
	return buf.empty() ? false : true;
}

#if 1
int tools::get_fds(pid_t pid)
{
	acl::string cmd;
	cmd.format("/bin/ls -l /proc/%d/fd|wc -l", (int) pid);

	int n;
	acl::string buf;
	if (exec_shell(cmd, buf) == false)
		n = -1;
	else
		n = atoi(buf.c_str());

	return n;
}

#else

int tools::get_fds(pid_t pid)
{
	acl::string buf;
	buf.format("/proc/%d/fd", (int) pid);

	ACL_ARGV* args = acl_argv_alloc(1);
	acl_argv_add(args, "/bin/ls", "-l", buf.c_str(), NULL);

	ACL_VSTREAM* fp = acl_vstream_popen(O_RDWR,
		ACL_VSTREAM_POPEN_ARGV, args->argv, ACL_VSTREAM_POPEN_END);
	if (fp == NULL)
	{
		logger_error("popen error=%s, cmd=ls /proc/%d/fd",
			acl::last_serror(), (int) pid);
		return -1;
	}

	int n = -1; // first line is invalid
	char line[1024];
	while (true)
	{
		int ret = acl_vstream_gets_nonl(fp, line, sizeof(line));
		if (ret == ACL_VSTREAM_EOF)
			break;
		n++;
	}

	acl_argv_free(args);
	acl_vstream_pclose(fp);

	return n;
}

#endif
