#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include "tools.h"

static bool check_mem(pid_t pid)
{
	long mem = tools::get_mem(pid);

	printf("pid=%d, mem=%lu MB, info=%s\r\n", (int) pid, mem,
		mem >= 0 ? "ok" : acl::last_serror());
	return true;
}

static bool check_fds(pid_t pid)
{
	int fds = tools::get_fds(pid);

	printf("pid=%d, fds=%d, info=%s\r\n", (int) pid, fds,
		fds >= 0 ? "ok" : acl::last_serror());
	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -a action[fds|mem|cpu|io|net] -p pid\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int ch;
	pid_t pid = getpid();
	acl::string action("fds");

	while ((ch = getopt(argc, argv, "ha:p:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'a':
			action = optarg;
			break;
		case 'p':
			pid = (pid_t) atoi(optarg);
			if (pid < 0)
				pid = getpid();
			break;
		default:
			break;
		}
	}

	bool ret;

	acl::log::stdout_open(true);

	if (action == "fds") {
		ret = check_fds(pid);
	} else if (action == "mem") {
		ret = check_mem(pid);
	} else {
		printf("action: %s not support yet!\r\n", action.c_str());
		return 1;
	}

	return ret ? 0 : 1;
}
