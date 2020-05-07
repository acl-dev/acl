#include "stdafx.h"
#include "util.h"
#include "websocket.h"

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
		log_info("delete websocket_client!\r\n");
		if (--__aio_refer == 0) {
			log_info("%s: stop aio engine now!\r\n", __FUNCTION__);
			handle_.stop();
		}
	}

	websocket_client& enable_debug(bool on)
	{
		debug_ = on;
		return *this;
	}

private:
	void show_ns_addr(void)
	{
		acl::string buf;
		if (this->get_ns_addr(buf)) {
			log_info(">>> current ns addr: %s\r\n", buf.c_str());
		} else {
			log_info(">>> current ns addr NULL\r\n");
		}
	}

protected:
	// @override
	void destroy(void)
	{
		log_info("%s(%d): websocket_client will be deleted!\r\n",
			__FUNCTION__, __LINE__);

		delete this;
	}

	// @override
	bool on_connect(void)
	{
		log_info("--------------- connect server ok ------------\r\n");
		show_ns_addr();
		log_info(">>> begin ws_handshake\r\n");

		this->ws_handshake();
		return true;
	}

	// @override
	void ws_handshake_before(acl::http_header& reqhdr)
	{
		acl::string buf;
		reqhdr.build_request(buf);
		log_info("---------------websocket request header---------\r\n");
		log_info("[%s]\r\n", buf.c_str());
	}

	// @override
	void on_disconnect(void)
	{
		log_info("%s(%d): disconnect from server\r\n",
			__FUNCTION__, __LINE__);
	}

	// @override
	void on_ns_failed(void)
	{
		log_info("dns lookup failed\r\n");
	}

	// @override
	void on_connect_timeout(void)
	{
		log_info("connect timeout\r\n");
		show_ns_addr();
	}

	// @override
	void on_connect_failed(void)
	{
		log_info("connect failed\r\n");
		show_ns_addr();
	}

	// @override
	bool on_read_timeout(void)
	{
		log_info("read timeout\r\n");
		return true;
	}

protected:
	// @override
	bool on_http_res_hdr(const acl::http_header& header)
	{
		acl::string buf;
		header.build_response(buf);

		compressed_ = header.is_transfer_gzip();

		log_info("-----------%s: response header----\r\n", __FUNCTION__);
		log_info("[%s]\r\n", buf.c_str());

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
		log_info(">>> websocket handshake ok\r\n");

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
		log_info(">>> websocket handshake failed, status=%d\r\n", status);
	}

	// @override
	bool on_ws_frame_text(void)
	{
		log_info(">>> got frame text type\r\n");
		return true;
	}

	// @override
	bool on_ws_frame_binary(void)
	{
		log_info(">>> got frame binaray type\r\n");
		return true;
	}

	// @override
	void on_ws_frame_closed(void)
	{
		log_info(">>> got frame closed type\r\n");
	}

	// @override
	bool on_ws_frame_data(char* data, size_t dlen)
	{
		acl::string buf;
		buf.copy(data, dlen);
		log_info("%s", buf.c_str());

		//(void) write(1, data, dlen);
		return true;
	}

	// @override
	bool on_ws_frame_finish(void)
	{
		log_info(">>> frame finish\r\n");
		return true;
	}

private:
	acl::string host_;
	bool debug_;
	bool compressed_;
};

bool websocket_run(void)
{
	int  conn_timeout = 5, rw_timeout = 5;
	acl::string addr("10.110.28.210:8885");
	acl::string host("www.baidu.com");
	std::vector<acl::string> name_servers;
	bool debug = false;

	// 定义 AIO 事件引擎
	acl::aio_handle handle(acl::ENGINE_KERNEL);

	//////////////////////////////////////////////////////////////////////

	if (name_servers.empty()) {
		name_servers.push_back("8.8.8.8:53");
	}

	for (std::vector<acl::string>::const_iterator cit = name_servers.begin();
		cit != name_servers.end(); ++cit) {

		// 设置 DNS 域名服务器地址
		handle.set_dns((*cit).c_str(), 5);
	}


	// 开始异步连接远程 WEB 服务器
	websocket_client* conn = new websocket_client(handle, host);
	if (!conn->open(addr, conn_timeout, rw_timeout)) {
		log_info("connect %s error\r\n", addr.c_str());

		delete conn;
		return false;
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
	log_info("---------------request header-----------------\r\n");
	log_info("[%s]\r\n", buf.c_str());

	// 开始 AIO 事件循环过程
	while (true) {
		// 如果返回 false 则表示不再继续，需要退出
		if (!handle.check()) {
			break;
		}
	}

	handle.check();
	return true;
}
