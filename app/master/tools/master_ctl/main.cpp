#include "stdafx.h"

static void print_server(const serv_info_t& server)
{
	printf("status: %d\r\n", server.status);
	printf("name: %s\r\n", server.name.c_str());
	printf("type: %d\r\n", server.type);
	printf("owner: %s\r\n", server.owner.c_str());
	printf("path: %s\r\n", server.path.c_str());
	printf("proc_max: %d\r\n", server.proc_max);
	printf("proc_prefork: %d\r\n", server.proc_prefork);
	printf("proc_total: %d\r\n", server.proc_total);
	printf("proc_avail: %d\r\n", server.proc_avail);
	printf("throttle_delay: %d\r\n", server.throttle_delay);
	printf("listen_fd_count: %d\r\n", server.listen_fd_count);
	printf("notify_addr: %s\r\n", server.notify_addr.c_str());
	printf("notify_recipients: %s\r\n", server.notify_recipients.c_str());
}

static void print_servers(const std::list<serv_info_t>& servers)
{
	for (std::list<serv_info_t>::const_iterator cit = servers.begin();
		cit != servers.end(); ++cit)
	{
		print_server(*cit);
		printf("-----------------------------------------------\r\n");
	}
}

static void print_servers(const std::vector<serv_info_t>& servers)
{
	for (std::vector<serv_info_t>::const_iterator cit = servers.begin();
		cit != servers.end(); ++cit)
	{
		print_server(*cit);
		printf("-----------------------------------------------\r\n");
	}
}

template<typename TReq, typename TRes>
bool http_request(const char* addr, const char* cmd, TReq& req, TRes& res)
{
	acl::string body;
	serialize<TReq>(req, body);

	acl::http_request conn(addr, 30, 30);
	acl::string url;
	url.format("/?cmd=%s", cmd);
	conn.request_header().set_url(url).set_keep_alive(false);
	if (conn.request(body, body.size()) == false)
	{
		printf("request error, json=[%s]\r\n", body.c_str());
		return false;
	}

	acl::json json;
	if (conn.get_body(json) == false)
	{
		printf("get_body error, json=[%s]\r\n", body.c_str());
		return false;
	}

	if (deserialize<TRes>(json, res) == false)
	{
		printf("deserialize error, res json=[%s], req json=[%s]\r\n",
			json.to_string().c_str(), body.c_str());
		return false;
	}

	return true;
}

static bool do_list(const char* addr, const char*)
{
	list_req_t req;
	req.cmd = "list";

	list_res_t res;
	if (!http_request<list_req_t, list_res_t>(addr, "list", req, res))
		return false;

	print_servers(res.data);
	return true;
}

static bool do_stat(const char* addr, const char* filepath)
{
	if (*filepath == 0)
	{
		printf("filepath null\r\n");
		return false;
	}

	stat_req_t req;
	req.cmd = "stat";
	stat_req_data_t req_data;
	req_data.path = filepath;
	req.data.push_back(req_data);

	stat_res_t res;
	if (!http_request<stat_req_t, stat_res_t>(addr, "stat", req, res))
		return false;

	print_servers(res.data);
	return true;
}

static void print_start_result(const start_res_data_t& data)
{
	printf("status: %d\r\n", data.status);
	printf("name: %s\r\n", data.name.c_str());
	printf("path: %s\r\n", data.path.c_str());
}

static void print_start_results(const std::vector<start_res_data_t>& data)
{
	for (std::vector<start_res_data_t>::const_iterator
		cit = data.begin(); cit != data.end(); ++cit)
	{
		print_start_result(*cit);
	}
}

static bool do_start(const char* addr, const char* filepath)
{
	if (*filepath == 0)
	{
		printf("filepath null\r\n");
		return false;
	}

	start_req_t req;
	start_req_data_t req_data;
	req_data.path = filepath;
	req.data.push_back(req_data);

	start_res_t res;
	if (!http_request<start_req_t, start_res_t>(addr, "start", req, res))
		return false;

	print_start_results(res.data);
	return true;
}

static void print_stop_result(const stop_res_data_t& data)
{
	printf("status: %d\r\n", data.status);
	printf("path: %s\r\n", data.path.c_str());
}

static void print_stop_results(const std::vector<stop_res_data_t>& data)
{
	for (std::vector<stop_res_data_t>::const_iterator cit = data.begin();
		cit != data.end(); ++cit)
	{
		print_stop_result(*cit);
	}
}

static bool do_stop(const char* addr, const char* filepath)
{
	if (*filepath == 0)
	{
		printf("filepath null\r\n");
		return false;
	}

	stop_req_t req;
	stop_req_data_t req_data;
	req_data.path = filepath;
	req.data.push_back(req_data);

	stop_res_t res;
	if (!http_request<stop_req_t, stop_res_t>(addr, "stop", req, res))
		return false;

	print_stop_results(res.data);
	return true;
}

static void print_reload(const reload_res_data_t& data)
{
	printf("status: %d\r\n", data.status);
	printf("proc_count: %d\r\n", data.proc_count);
	printf("proc_signaled: %d\r\n", data.proc_signaled);
	printf("path: %s\r\n", data.path.c_str());
}

static void print_reload_results(const std::vector<reload_res_data_t>& res)
{
	for (std::vector<reload_res_data_t>::const_iterator cit = res.begin();
		cit != res.end(); ++cit)
	{
		print_reload(*cit);
	}
}

static bool do_reload(const char* addr, const char* filepath)
{
	if (*filepath == 0)
	{
		printf("filepath null\r\n");
		return false;
	}

	reload_req_t req;
	reload_req_data_t req_data;
	req_data.path = filepath;
	req.data.push_back(req_data);

	reload_res_t res;
	if (!http_request<reload_req_t, reload_res_t>(addr, "reload", req, res))
		return false;

	print_reload_results(res.data);
	return true;
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
	acl::string filepath, action("list"), addr("127.0.0.1:8190");
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
	bool ret;

	if (action == "list")
		ret = do_list(addr, filepath);
	else if (action == "stat")
		ret = do_stat(addr, filepath);
	else if (action == "start")
		ret = do_start(addr, filepath);
	else if (action == "stop")
		ret = do_stop(addr, filepath);
	else if (action == "reload")
		ret = do_reload(addr, filepath);
	else
	{
		printf("unknown action: %s\r\n", action.c_str());
		usage(argv[0]);
		return 1;
	}

	printf("----action: %s, result: %s----\r\n", action.c_str(),
		ret ? "ok" : "error");
	return 0;
}
