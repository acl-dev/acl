#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include "acl_cpp/lib_acl.hpp"

static acl::atomic_long __aio_refer = 0;

//////////////////////////////////////////////////////////////////////////////

class websocket_client : public acl::http_aclient
{
public:
	websocket_client(acl::aio_handle& handle, const char* host)
	: http_aclient(handle, NULL)
	, host_(host)
	, debug_(false)
	, compressed_(false)
	{
		++__aio_refer;
	}

	~websocket_client(void)
	{
		printf("delete websocket_client!\r\n");
		if (--__aio_refer == 0) {
			printf("%s: stop aio engine now!\r\n", __FUNCTION__);
			handle_.stop();
		}
	}

	websocket_client& enable_debug(bool on)
	{
		debug_ = on;
		return *this;
	}

protected:
	// @override
	void destroy(void)
	{
		printf("%s(%d): websocket_client will be deleted!\r\n",
			__FUNCTION__, __LINE__);
		fflush(stdout);

		delete this;
	}

	// @override
	bool on_connect(void)
	{
		printf("--------------- connect server ok ------------\r\n");
		fflush(stdout);

		printf(">>> begin ws_handshake\r\n");
		this->ws_handshake();
		return true;
	}

	// @override
	void ws_handshake_before(acl::http_header& reqhdr)
	{
		acl::string buf;
		reqhdr.build_request(buf);
		printf("---------------websocket request header---------\r\n");
		printf("[%s]\r\n", buf.c_str());
		fflush(stdout);
	}

	// @override
	void on_disconnect(void)
	{
		printf("%s(%d): disconnect from server\r\n",
			__FUNCTION__, __LINE__);
		fflush(stdout);
	}

	// @override
	void on_connect_timeout(void)
	{
		printf("connect timeout\r\n");
		fflush(stdout);
	}

	// @override
	void on_connect_failed(void)
	{
		printf("connect failed\r\n");
		fflush(stdout);
	}

	// @override
	bool on_read_timeout(void)
	{
		printf("read timeout\r\n");
		return true;
	}

	// @override
	bool on_http_res_hdr(const acl::http_header& header)
	{
		acl::string buf;
		header.build_response(buf);

		compressed_ = header.is_transfer_gzip();

		printf("-----------%s: response header----\r\n", __FUNCTION__);
		printf("[%s]\r\n", buf.c_str());
		fflush(stdout);

		return true;
	}

#if 0
	// @override
	bool on_http_res_body(char* data, size_t dlen)
	{
		if (debug_ && (!compressed_ || this->is_unzip_body())) {
			(void) write(1, data, dlen);
		} else {
			printf(">>>read body: %ld\r\n", dlen);
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

		return keep_alive_;
	}
#endif

protected:
	// @override
	bool on_ws_handshake(void)
	{
		printf(">>> websocket handshake ok\r\n");
		fflush(stdout);

		char buf[128];
		snprintf(buf, sizeof(buf), "hello, myname is zsx\r\n");
		size_t len = strlen(buf);

		if (!this->ws_send_text(buf, len)) {
			return false;
		}

		// 开始进入 websocket 异步读过程
		this->ws_read_wait(5);
		return true;
	}

	// @override
	void on_ws_handshake_failed(int status)
	{
		printf(">>> websocket handshake failed, status=%d\r\n", status);
		fflush(stdout);
	}

	// @override
	bool on_ws_frame_text(void)
	{
		printf(">>> got frame text type\r\n");
		fflush(stdout);
		return true;
	}

	// @override
	bool on_ws_frame_binary(void)
	{
		printf(">>> got frame binaray type\r\n");
		fflush(stdout);
		return true;
	}

	// @override
	void on_ws_frame_closed(void)
	{
		printf(">>> got frame closed type\r\n");
		fflush(stdout);
	}

	// @override
	bool on_ws_frame_data(char* data, size_t dlen)
	{
		(void) write(1, data, dlen);
		return true;
	}

	// @override
	bool on_ws_frame_finish(void)
	{
		printf(">>> frame finish\r\n");
		fflush(stdout);
		return true;
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
		" -D [if in debug mode, default: false]\r\n"
		" -c cocorrent\r\n"
		" -t connect_timeout[default: 5]\r\n"
		" -i rw_timeout[default: 5]\r\n"
		" -N name_server[default: 8.8.8.8:53]\r\n"
		, procname);
}

int test_websocket_main(int argc, char* argv[])
{
	int  ch, conn_timeout = 5, rw_timeout = 5;
	acl::string addr("127.0.0.1:80"), name_server("8.8.8.8:53");
	acl::string host("www.baidu.com");
	bool debug = false;

	while ((ch = getopt(argc, argv, "hs:N:H:t:i:D")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 's':
			addr = optarg;
			break;
		case 'N':
			name_server = optarg;
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
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	// 定义 AIO 事件引擎
	acl::aio_handle handle(acl::ENGINE_KERNEL);

	//////////////////////////////////////////////////////////////////////

	// 设置 DNS 域名服务器地址
	handle.set_dns(name_server.c_str(), 5);

	// 开始异步连接远程 WEB 服务器
	websocket_client* conn = new websocket_client(handle, host);
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
	head.set_url("/path?name1&name2")
		.add_param("name3", "")
		.add_param("n1", "v1")
		.add_param("n2", "v2")
		.add_param("n3", "")
		.set_content_length(0)
		.set_host(host)
		.accept_gzip(true)
		.set_keep_alive(true);

	acl::string buf;
	head.build_request(buf);
	printf("---------------request header-----------------\r\n");
	printf("[%s]\r\n", buf.c_str());
	fflush(stdout);

	// 开始 AIO 事件循环过程
	while (true) {
		// 如果返回 false 则表示不再继续，需要退出
		if (!handle.check()) {
			break;
		}
	}

	handle.check();
	return 0;
}
