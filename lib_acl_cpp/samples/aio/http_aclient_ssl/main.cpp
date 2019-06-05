#include <iostream>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
//#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

static acl::polarssl_conf* __ssl_conf;
static int __conn_timeout = 5;

class http_aio_client : public acl::http_aclient
{
public:
	http_aio_client(acl::aio_handle& handle, acl::polarssl_conf* ssl_conf,
		const char* host)
	: http_aclient(handle, ssl_conf)
	//, keep_alive_(true)
	, keep_alive_(false)
	, host_(host)
	{
	}

	~http_aio_client(void)
	{
		handle_.stop();
	}

protected:
	// @override
	bool on_connect(void)
	{
		acl::http_header& head = this->request_header();
		head.set_url("/")
			.set_content_length(0)
			.set_host(host_)
			.accept_gzip(true)
			//.accept_gzip(false)
			.set_keep_alive(keep_alive_);

		acl::string buf;
		head.build_request(buf);
		printf("---------------request header-----------------\r\n");
		printf("[%s]\r\n", buf.c_str());
		fflush(stdout);

		this->send_request(NULL, 0);
		return true;
	}

	// @override
	void on_disconnect(void)
	{
		printf("disconnect from server\r\n");
		fflush(stdout);

		delete this;
	}

	// @override
	void on_connect_timeout(void)
	{
	}

	// @override
	void on_connect_failed(void)
	{
		printf("connect failed\r\n");
		fflush(stdout);

		delete this;
	}

	// @override
	bool on_http_res_hdr(const acl::http_header& header)
	{
		acl::string buf;
		header.build_response(buf);

		printf("---------------response header-----------------\r\n");
		printf("[%s]\r\n", buf.c_str());
		fflush(stdout);

		keep_alive_ = header.get_keep_alive();
		return true;
	}

	// @override
	bool on_http_res_body(char* data, size_t dlen)
	{
		if (0) {
			(void) write(1, data, dlen);
		} else {
			printf(">>>read body: %ld\r\n", dlen);
		}
		return true;
	}

	// @override
	bool on_http_res_finish(void)
	{
		printf("---------------response over-------------------\r\n");
		printf("http finish: keep_alive=%s\r\n",
			keep_alive_ ? "true" : "false");
		fflush(stdout);

		return keep_alive_;
	}

private:
	bool keep_alive_;
	acl::string host_;
};

static bool connect_server(acl::aio_handle& handle, const char* addr,
	const char* host)
{
	http_aio_client* conn = new http_aio_client(handle, __ssl_conf, host);
	if (!conn->open(addr, __conn_timeout)) {
		printf("connect %s error\r\n", addr);
		fflush(stdout);

		delete conn;
		return false;
	}

	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		" -s server_addr\r\n"
		" -c cocorrent\r\n"
		" -t connect_timeout\r\n"
		" -S polarssl_lib_path[default: none]\n"
		" -N name_server[default: 8.8.8.8:53]\r\n"
		, procname);
}

int main(int argc, char* argv[])
{
	int  ch;
	acl::string addr("127.0.0.1:80"), name_server("8.8.8.8:53");
	acl::string host("www.baidu.com"), ssl_lib_path;

	while ((ch = getopt(argc, argv, "hs:S:N:H:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 's':
			addr = optarg;
			break;
		case 'S':
			ssl_lib_path = optarg;
			break;
		case 'N':
			name_server = optarg;
			break;
		case 'H':
			host = optarg;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	if (!ssl_lib_path.empty()) {
		if (access(ssl_lib_path.c_str(), R_OK) == 0) {
			__ssl_conf = new acl::polarssl_conf;
		} else {
			printf("disable ssl, %s not found\r\n",
				ssl_lib_path.c_str());
		}
	}

	acl::aio_handle handle(acl::ENGINE_KERNEL);

	handle.set_dns(name_server.c_str(), 5);

	if (!connect_server(handle, addr, host)) {
		return 1;
	}

	while (true) {
		// 如果返回 false 则表示不再继续，需要退出
		if (!handle.check()) {
			break;
		}
	}

	handle.check();
	delete __ssl_conf;
	return (0);
}

