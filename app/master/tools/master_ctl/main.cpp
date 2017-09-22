#include "stdafx.h"
#include <readline/readline.h>
#include <readline/history.h>
#include "http_request.h"

static bool __verbose = false;
static long long __timeout = 0;

static void print_space(int n)
{
	for (int i = 0; i < n; i++)
		putchar(' ');
	printf("\r\n");
}

static void print_name(const char* name, int tabs = 1)
{
	printf("\033[1;33;40m%-s\033[0m", name);

	for (int i = 0; i < tabs; i++)
		printf("\t");
}

static void print_value(const char* value, int tabs = 1)
{
	printf("\033[1;36;40m%-s\033[0m", value);

	for (int i = 0; i < tabs; i++)
		printf("\t");
}

static void print_value(int value, int tabs = 1)
{
	char buf[32];
	snprintf(buf, sizeof(buf), "%d", value);
	print_value(buf, tabs);
}

static void print_head(void)
{
	print_name("status");
	print_name("service", 2);
	print_name("type");
	print_name("proc_count", 1);
	print_name("owner");
	print_name("conf");
	printf("\r\n");
}

static void print_server(const serv_info_t& server)
{
	print_value(server.status);
	print_value(server.name, 2);
	print_value(server.type);
	print_value(server.proc_total, 2);
	print_value(server.owner);
	print_value(server.conf);
	printf("\r\n");
}

static void println_underline(const char* name, const char* value)
{
	printf("\033[4m\033[1;36;40m%c\033[0m\033[0m", *name);
	name++;
	printf("\033[1;33;40m%-18s\033[0m: %s\r\n", name, value);
}

static void println(const char* name, const char* value)
{
	printf("\033[1;33;40m%-18s\033[0m: %s\r\n", name, value);
}

static void println(const char* name, int value)
{
	char buf[32];

	snprintf(buf, sizeof(buf), "%d", value);
	println(name, buf);
}

static const char* time_format(time_t t, char* buf, size_t size)
{
	struct tm local_time;
	localtime_r(&t, &local_time);
	strftime(buf, size, "%Y/%m/%d-%H:%M:%S", &local_time);
	return buf;
}

static void println_server(const serv_info_t& server)
{
	println("status", server.status);
	println("name", server.name.c_str());
	println("type", server.type);
	println("owner", server.owner.c_str());
	println("path", server.path.c_str());
	println("conf", server.conf.c_str());
	println("proc_max", server.proc_max);
	println("proc_prefork", server.proc_prefork);
	println("proc_total", server.proc_total);
	println("proc_avail", server.proc_avail);
	println("throttle_delay", server.throttle_delay);
	println("listen_fd_count", server.listen_fd_count);
	println("notify_addr", server.notify_addr.c_str());
	println("notify_recipients", server.notify_recipients.c_str());

	acl::string buf;
	size_t i = 0;

	for (std::list<proc_info_t>::const_iterator cit = server.procs.begin();
		cit != server.procs.end(); ++cit)
	{
		if (i++ > 0)
			buf << "                    ";

		char tmp[128];
		buf << "pid=" << (*cit).pid << ", start="
			<< time_format((time_t) (*cit).start, tmp, sizeof(tmp));
		buf += ";\r\n";
	}

	println("processes", buf);
}

static void print_servers(const std::vector<serv_info_t>& servers, bool verbose)
{
	if (!verbose)
		print_head();

	for (std::vector<serv_info_t>::const_iterator cit = servers.begin();
		cit != servers.end(); ++cit)
	{
		if (!verbose)
		{
			print_server(*cit);
			continue;
		}

		if (cit != servers.begin())
			print_space(100);
		println_server(*cit);
	}
}

static bool do_list(const std::vector<acl::string>&,
	const char* addr, const char*)
{
	list_req_t req;
	req.cmd = "list";

	list_res_t res;
	if (!http_request<list_req_t, list_res_t>(addr, req, res))
		return false;

	print_servers(res.data, __verbose);

	char buf[32];
	snprintf(buf, sizeof(buf), "\r\nservers count: %d\r\n",
		(int) res.data.size());
	print_name(buf);
	return true;
}

static bool do_stat(const std::vector<acl::string>&,
	const char* addr, const char* fpath)
{
	if (*fpath == 0)
	{
		printf("filepath null\r\n");
		return false;
	}

	stat_req_t req;
	req.cmd = "stat";

	stat_req_data_t req_data;
	req_data.path = fpath;
	req.data.push_back(req_data);

	stat_res_t res;
	if (!http_request<stat_req_t, stat_res_t>(addr, req, res))
		return false;

	print_servers(res.data, true);
	return true;
}

static void print_start_result(const start_res_data_t& data)
{
	println("status", data.status);
	println("name", data.name.c_str());
	println("path", data.path.c_str());
}

static void print_start_results(const std::vector<start_res_data_t>& data)
{
	for (std::vector<start_res_data_t>::const_iterator
		cit = data.begin(); cit != data.end(); ++cit)
	{
		print_start_result(*cit);
	}
}

static bool do_start(const std::vector<acl::string>&,
	const char* addr, const char* fpath)
{
	if (*fpath == 0)
	{
		printf("filepath null\r\n");
		return false;
	}

	start_req_t req;
	req.cmd = "start";

	start_req_data_t req_data;
	req_data.path = fpath;
	req.data.push_back(req_data);

	start_res_t res;
	if (!http_request<start_req_t, start_res_t>(addr, req, res))
		return false;

	print_start_results(res.data);
	return true;
}

static void print_restart_result(const restart_res_data_t& data)
{
	println("status", data.status);
	println("name", data.name.c_str());
	println("path", data.path.c_str());
}

static void print_restart_results(const std::vector<restart_res_data_t>& data)
{
	for (std::vector<restart_res_data_t>::const_iterator
		cit = data.begin(); cit != data.end(); ++cit)
	{
		print_restart_result(*cit);
	}
}

static bool do_restart(const std::vector<acl::string>&,
	const char* addr, const char* fpath)
{
	if (*fpath == 0)
	{
		printf("filepath null\r\n");
		return false;
	}

	restart_req_t req;
	req.cmd = "restart";

	restart_req_data_t req_data;
	req_data.path = fpath;
	req.data.push_back(req_data);

	restart_res_t res;
	if (!http_request<restart_req_t, restart_res_t>(addr, req, res))
		return false;

	print_restart_results(res.data);
	return true;
}

static void print_stop_result(const stop_res_data_t& data)
{
	println("status", data.status);
	println("path", data.path.c_str());
}

static void print_stop_results(const std::vector<stop_res_data_t>& data)
{
	for (std::vector<stop_res_data_t>::const_iterator cit = data.begin();
		cit != data.end(); ++cit)
	{
		print_stop_result(*cit);
	}
}

static bool do_stop(const std::vector<acl::string>&,
	const char* addr, const char* fpath)
{
	if (*fpath == 0)
	{
		printf("filepath null\r\n");
		return false;
	}

	stop_req_t req;
	req.cmd = "stop";

	stop_req_data_t req_data;
	req_data.path = fpath;
	req.data.push_back(req_data);

	stop_res_t res;
	if (!http_request<stop_req_t, stop_res_t>(addr, req, res))
		return false;

	print_stop_results(res.data);
	return true;
}

static void print_kill_result(const kill_res_data_t& data)
{
	println("status", data.status);
	println("path", data.path.c_str());
}

static void print_kill_results(const std::vector<kill_res_data_t>& data)
{
	for (std::vector<kill_res_data_t>::const_iterator cit = data.begin();
		cit != data.end(); ++cit)
	{
		print_kill_result(*cit);
	}
}

static bool do_kill(const std::vector<acl::string>&,
	const char* addr, const char* fpath)
{
	if (*fpath == 0)
	{
		printf("filepath null\r\n");
		return false;
	}

	kill_req_t req;
	req.cmd = "kill";

	kill_req_data_t req_data;
	req_data.path = fpath;
	req.data.push_back(req_data);

	kill_res_t res;
	if (!http_request<kill_req_t, kill_res_t>(addr, req, res))
		return false;

	print_kill_results(res.data);
	return true;
}

static void print_reload(const reload_res_data_t& data)
{
	println("status", data.status);
	println("proc_count", data.proc_count);
	println("proc_signaled", data.proc_signaled);
	println("proc_ok", data.proc_ok);
	println("path", data.path.c_str());
}

static void print_reload_results(const std::vector<reload_res_data_t>& res)
{
	for (std::vector<reload_res_data_t>::const_iterator cit = res.begin();
		cit != res.end(); ++cit)
	{
		print_reload(*cit);
	}
}

static bool do_reload(const std::vector<acl::string>& tokens,
	const char* addr, const char* fpath)
{
	if (*fpath == 0)
	{
		printf("\033[1;34;40musage\033[0m: "
			"\033[1;33;40mreload\033[0m configure_path timeout\r\n");
		return false;
	}

	if (tokens.size() >= 3)
		__timeout = atoll(tokens[2]);

	reload_req_t req;
	req.cmd = "reload";
	req.timeout = __timeout;

	reload_req_data_t req_data;
	req_data.path = fpath;
	req.data.push_back(req_data);

	reload_res_t res;
	if (!http_request<reload_req_t, reload_res_t>(addr, req, res))
		return false;

	print_reload_results(res.data);
	return true;
}

static bool do_set(const std::vector<acl::string>& tokens,
	const char*, const char* fpath)
{
	if (*fpath == 0)
	{
		printf("\033[1;34;40musage\033[0m: "
			"\033[1;33;40mset\033[0m configure_path timeout\r\n");
		return true;
	}

	if (tokens.size() >= 3)
		__timeout = atoll(tokens[2]);

	printf("set service to %s ok, timeout=%lld\r\n", fpath, __timeout);
	return true;
}

static bool do_server(const std::vector<acl::string>& tokens,
	const char* addr, const char*)
{
	if (tokens.size() > 1)
	{
		addr = tokens[1].c_str();
		printf("set server to %s ok\r\n", addr);
	}
	else if (*addr == 0)
		printf("\033[1;34;40musage\033[0m: "
			"\033[1;33;40mserver\033[0m addr\r\n");
	else
		printf("server addr is %s\r\n", addr);
	return true;
}

static bool do_verbose(const std::vector<acl::string>&,
	const char*, const char*)
{
	__verbose = __verbose ? false : true;
	printf("set verbose %s\r\n", __verbose ? "on" : "off");
	return true;
}

static bool do_help(const std::vector<acl::string>&, const char*, const char*)
{
	println("set", "set the service's configure");
	println("server", "set the service's addr");
	println_underline("verbose", "display verbose");
	println_underline("clear", "clear the screan");
	println_underline("list", "list all the running services");
	println_underline("stat", "show one service's running status");
	println("start", "start one service");
	println("restart", "restart one service");
	println("stop", "stop one service");
	println("kill", "kill one service");
	println("reload", "reload one service");

	return true;
}

static bool do_quit(const std::vector<acl::string>&, const char*, const char*)
{
	printf("Bye!\r\n");
	exit(0);

	// not reached here
	return false;
}

static bool do_clear(const std::vector<acl::string>&, const char*, const char*)
{
#if	!defined(__APPLE__)
	rl_clear_screen(0, 0);
#endif
	printf("\r\n");
	return true;
}

static void getline(acl::string& out)
{
	const char* prompt = "\033[1;34;40mmaster_ctl>\033[0m ";
	char* ptr = readline(prompt);
	if (ptr == NULL)
	{
		printf("Bye!\r\n");
		exit(0);
	}
	out = ptr;
	out.trim_right_line();
	if (!out.empty() && !out.equal("y", false) && !out.equal("n", false))
		add_history(out.c_str());
}

static struct {
	const char* cmd;
	char  shortcut;
	bool  has_path;
	bool (*fn)(const std::vector<acl::string>&, const char*, const char*);
} __actions[] = {
	{ "list",	'l',	false,	do_list		},
	{ "stat",	's',	true,	do_stat		},
	{ "start",	'\0',	true,	do_start	},
	{ "restart",	'\0',	true,	do_restart	},
	{ "stop",	'\0',	true,	do_stop		},
	{ "kill",	'\0',	true,	do_kill		},
	{ "reload",	'r',	true,	do_reload	},

	{ "help",	'h',	false,	do_help		},
	{ "clear",	'c',	false,	do_clear	},
	{ "quit",	'q',	false,	do_quit		},
	{ "exit",	'e',	false,	do_quit		},
	{ "set",	'\0',	true,	do_set		},
	{ "server",	'\0',	false,	do_server	},
	{ "verbose",	'v',	false,	do_verbose	},

	{ 0,		0,	false,	0		},
};

static void run(const char* server, const char* filepath)
{
	acl::string buf, fpath, addr(server);

	printf("server addr is %s\r\n", server);

	if (filepath && *filepath)
		fpath = filepath;

	while (true)
	{
		getline(buf);
		if (buf.empty())
			continue;

		std::vector<acl::string>& tokens = buf.split2(" \t", true);
		acl::string cmd = tokens[0];
		cmd.lower();

		bool ret = false;
		int  i;
		for (i = 0; __actions[i].cmd; i++)
		{
			if (cmd == __actions[i].cmd ||
				(__actions[i].shortcut &&
				  cmd.size() == 1 &&
				  cmd[0] == __actions[i].shortcut))
			{
				break;
			}
		}

		if (__actions[i].cmd == NULL)
		{
			do_help(tokens, addr, fpath);
			continue;
		}

		if (__actions[i].has_path && tokens.size() >= 2)
			fpath = tokens[1];

		ret = __actions[i].fn(tokens, addr, fpath);
		if (!__verbose)
			print_space(100);

		printf("\033[1;36;40m%s\033[0m ==> \033[1;32;40m%s\033[0m\r\n",
			__actions[i].cmd, ret ? "ok" : "err");
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		" -s master_manage_addr[default: 127.0.0.1:8190]\r\n"
		" -f servicde_path\r\n"
		" -a cmd[list|stat|start|stop|reload]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	acl::string filepath, action, addr("127.0.0.1:8290");
	int  ch;

	while ((ch = getopt(argc, argv, "hs:f:a:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'f':
			filepath = optarg;
			break;
		case 'a':
			action = optarg;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	action.lower();

        acl::log::stdout_open(true);

	std::vector<acl::string> tokens;

	int i;
	for (i = 0; __actions[i].cmd; i++)
	{
		if (action == __actions[i].cmd)
		{
			(void) __actions[i].fn(tokens, addr, filepath);
			break;
		}
	}

	if (__actions[i].cmd == NULL)
		run(addr, filepath);

	return 0;
}
