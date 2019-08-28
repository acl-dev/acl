#include "stdafx.h"
#include "http_client.h"

acl::atomic_long __aio_refer = 0;
int __success         = 0;
int __destroy         = 0;
int __disconnect      = 0;
int __ns_failed       = 0;
int __connect_ok      = 0;
int __connect_timeout = 0;
int __connect_failed  = 0;
int __header_ok       = 0;
int __read_timeout    = 0;

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		" -s server_addr\r\n"
		" -e event_type[kernel|select|poll, default: kernel]\r\n"
		" -D [if in debug mode, default: false]\r\n"
		" -c cocorrent\r\n"
		" -t connect_timeout[default: 5]\r\n"
		" -i rw_timeout[default: 5]\r\n"
		" -U url\r\n"
		" -H host\r\n"
		" -K [http keep_alive true]\r\n"
		" -N name_server_list[default: 8.8.8.8:53]\r\n"
		" -T ns_lookup_timeout[default: 5]\r\n"
		, procname);
}

static void add_name_servers(std::vector<acl::string>& addrs, const char* s)
{
	acl::string buf(s);
	const std::vector<acl::string>& tokens = buf.split2(",; \t");

	for (std::vector<acl::string>::const_iterator cit = tokens.begin();
		cit != tokens.end(); ++cit) {

		addrs.push_back(*cit);
	}
}

int main(int argc, char* argv[])
{
	int  ch, ns_timeout = 5, conn_timeout = 5, rw_timeout = 5, cocurrent = 1;
	acl::string addr("pvwu8bubc.bkt.clouddn.com:80");
	acl::string host("pvwu8bubc.bkt.clouddn.com"), url("/20160528212429_c2HAm.jpeg");
	std::vector<acl::string> name_servers;
	bool debug = false, keep_alive = false;
	acl::string event("kernel");
	acl::aio_handle_type event_type;

	while ((ch = getopt(argc, argv, "he:Kc:s:N:U:H:t:i:DT:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 'e':
			event = optarg;
			break;
		case 'K':
			keep_alive = true;
			break;
		case 'c':
			cocurrent = atoi(optarg);
			break;
		case 's':
			addr = optarg;
			break;
		case 'N':
			add_name_servers(name_servers, optarg);
			break;
		case 'U':
			url = optarg;
			break;
		case 'H':
			host = optarg;
			break;
		case 't':
			conn_timeout = atoi(optarg);
			break;
		case 'i':
			rw_timeout = atoi(optarg);
			break;
		case 'D':
			debug = true;
			break;
		case 'T':
			ns_timeout = atoi(optarg);
			break;
		default:
			break;
		}
	}

	if (name_servers.empty()) {
		name_servers.push_back("8.8.8.8:53");
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	if (event == "select") {
		event_type = acl::ENGINE_SELECT;
		printf("use select event\r\n");
	} else if (event == "poll") {
		event_type = acl::ENGINE_POLL;
		printf("use poll event\r\n");
	} else {
		event_type = acl::ENGINE_KERNEL;
		printf("use kernel event\r\n");
	}

	// 定义 AIO 事件引擎
	acl::aio_handle handle(event_type);

	handle.set_delay_sec(0);
	handle.set_delay_usec(1000000);

	// 设置 DNS 域名服务器地址
	for (std::vector<acl::string>::const_iterator cit = name_servers.begin();
		cit != name_servers.end(); ++cit) {
		handle.set_dns(*cit, ns_timeout);
	}

	//////////////////////////////////////////////////////////////////////

	// 开始异步连接远程 WEB 服务器
	for (int i = 0; i < cocurrent; i++) {
		http_client* conn = new http_client(handle);
		(*conn).set_addr(addr)
			.set_timeout(conn_timeout, rw_timeout)
			.set_url(url)
			.set_debug(debug)
			.set_host(host)
			.set_keep_alive(keep_alive);

		// 设置重定向最大次数，如果此值为 0 则禁止重定向功能
		conn->set_redirect_limit(3);

		if (!conn->start()) {
			printf("connect %s error\r\n", addr.c_str());
			return 1;
		}
	}

	time_t last = time(NULL), now, begin = last;
	// 开始 AIO 事件循环过程
	while (true) {
		// 如果返回 false 则表示不再继续，需要退出
		if (!handle.check()) {
			break;
		}
		(void) time(&now);
		if (now - last > 0) {
			printf("continue check %ld seconds...\r\n", now - last);
		}
		last = now;
	}

	(void) time(&now);
	printf("\r\ntime spent: %ld seconds\r\n", now - begin);

	handle.check();

	printf("\r\n---------------------------------------------------\r\n");

	printf("all over, destroy=%d\r\n\r\n", __destroy);
	printf("ns_failed=%d, connect_ok=%d, disconnect=%d, connect_timeout=%d,"
		" connect_faile=%d\r\n\r\n", __ns_failed, __connect_ok,
		__disconnect, __connect_timeout, __connect_failed);
	printf("success=%d, header_ok=%d, read_timeout=%d\r\n",
		__success, __header_ok, __read_timeout);

	printf("---------------------------------------------------\r\n");
	return 0;
}
