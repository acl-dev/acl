#include <iostream>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

static acl::atomic_long __aio_refer = 0;
static int __success = 0, __destroy = 0, __disconnect = 0, __ns_failed = 0;
static int __connect_ok = 0, __connect_timeout = 0, __connect_failed = 0;
static int __header_ok = 0, __read_timeout = 0;

//////////////////////////////////////////////////////////////////////////////

class http_client : public acl::http_aclient
{
public:
	http_client(acl::aio_handle& handle, const char* host)
	: http_aclient(handle, NULL)
	, host_(host)
	, debug_(false)
	, compressed_(false)
	{
		++__aio_refer;
	}

	~http_client(void)
	{
		printf("delete http_client!\r\n");
		if (--__aio_refer == 0) {
			printf("%s: stop aio engine now!\r\n", __FUNCTION__);
			handle_.stop();
		}
	}

	http_client& enable_debug(bool on)
	{
		debug_ = on;
		return *this;
	}

protected:
	// @override
	void destroy(void)
	{
		printf("http_client will be deleted!\r\n");
		fflush(stdout);

		__destroy++;
		delete this;
	}

	// @override
	bool on_connect(void)
	{
		printf("--------------- connect server ok ------------\r\n");
		acl::string addr;
		if (this->get_ns_addr(addr)) {
			printf(">>>ns server: %s\r\n", addr.c_str());
		}

		fflush(stdout);

		printf(">>> begin send_request\r\n");
		//this->ws_handshake();
		this->send_request(NULL, 0);

		__connect_ok++;
		return true;
	}

	// @override
	void on_disconnect(void)
	{
		printf("disconnect from server\r\n");
		fflush(stdout);
		__disconnect++;
	}

	// @override
	void on_ns_failed(void)
	{
		printf("dns lookup failed\r\n");
		fflush(stdout);
		__ns_failed++;
	}

	// @override
	void on_connect_timeout(void)
	{
		printf("connect timeout\r\n");
		fflush(stdout);
		__connect_timeout++;
	}

	// @override
	void on_connect_failed(void)
	{
		printf("connect failed\r\n");
		fflush(stdout);
		__connect_failed++;
	}

	// @override
	bool on_read_timeout(void)
	{
		printf("read timeout\r\n");
		__read_timeout++;
		return false;
	}

protected:
	// @override
	bool on_http_res_hdr(const acl::http_header& header)
	{
		acl::string buf;
		header.build_response(buf);

		compressed_ = header.is_transfer_gzip();
		__header_ok++;

		if (!debug_) {
			return true;
		}
		printf("-----------%s: response header----\r\n", __FUNCTION__);
		printf("[%s]\r\n", buf.c_str());
		fflush(stdout);

		return true;
	}

	// @override
	bool on_http_res_body(char* data, size_t dlen)
	{
		if (!debug_) {
			return true;
		}

		if (!compressed_ || this->is_unzip_body()) {
			(void) write(1, data, dlen);
		} else {
			printf(">>>read body: %ld\r\n", (long) dlen);
		}
		return true;
	}

	// @override
	bool on_http_res_finish(bool success)
	{
		printf("---------------response over-------------------\r\n");
		printf("http finish: keep_alive=%s, success=%s\r\n",
			keep_alive_ ? "true" : "false",
			success ? "ok" : "failed");
		fflush(stdout);
		__success++;

		return keep_alive_;
	}

private:
	acl::string host_;
	bool debug_;
	bool compressed_;
};

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
		http_client* conn = new http_client(handle, host);
		if (!conn->open(addr, conn_timeout, rw_timeout)) {
			printf("connect %s error\r\n", addr.c_str());
			fflush(stdout);

			delete conn;
			return 1;
		}

		(*conn).enable_debug(debug);		// 是否启用调试方式
		conn->unzip_body(true);			// 针对 HTTP 自动解压

		// 设置 HTTP 请求头，也可将此过程放在 conn->on_connect() 里
		acl::http_header& head = conn->request_header();
		head.set_url(url)
			.set_host(host)
			.accept_gzip(true)
			.set_keep_alive(keep_alive);

		if (i > 0) {
			continue;
		}

		acl::string buf;
		head.build_request(buf);
		printf("---------------request header-----------------\r\n");
		printf("[%s]\r\n", buf.c_str());
		fflush(stdout);
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
