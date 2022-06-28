#include <iostream>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

static acl::atomic_long __aio_refer = 0;

//////////////////////////////////////////////////////////////////////////////

#ifndef USE_AIO_OSTREAM

class pipe_ostream : public acl::aio_ostream
{
public:
	pipe_ostream(acl::aio_handle& handle, int fd)
	: aio_stream(&handle), aio_ostream(&handle, fd)
	, handle_(handle)
	{
	}

	~pipe_ostream(void)
	{
		if (--__aio_refer == 0) {
			printf("%s: stop aio engine now!\r\n", __FUNCTION__);
			handle_.stop();
		}
	}

protected:
	// @override
	void destroy(void)
	{
		printf("pipe_ostream: will be destroied!\r\n");
		delete this;
	}

private:
	acl::aio_handle& handle_;
};

#endif

class pipe_writer : public acl::aio_callback
{
public:
	pipe_writer(acl::aio_handle& handle, int fd)
	: handle_(handle)
	{
#ifdef USE_AIO_OSTREAM
		out_ = new acl::aio_ostream(&handle_, fd);
#else
		out_ = new pipe_ostream(handle_, fd);
#endif
		++__aio_refer;
	}

	void start(void)
	{
		out_->add_write_callback(this);
		out_->add_close_callback(this);
	}

	acl::aio_ostream& get_ostream(void) const
	{
		return *out_;
	}

protected:
	// @override
	bool write_callback(void)
	{
		return true;
	}

	// @override
	void close_callback(void)
	{
		printf("pipe_writer->being closed!\r\n");
		fflush(stdout);
		delete this;
	}

private:
	acl::aio_handle&  handle_;
#ifdef USE_AIO_OSTREAM
	acl::aio_ostream* out_;
#else
	pipe_ostream*     out_;
#endif

	~pipe_writer(void)
	{
		printf("%s: writer pipe will deleted!\r\n", __FUNCTION__);
	}
};

class pipe_reader : public acl::aio_callback
{
public:
	pipe_reader(acl::aio_handle& handle, int fd, pipe_writer& out)
	: handle_(handle)
	, out_(out)
	{
		in_ = new acl::aio_istream(&handle, fd);
		++__aio_refer;
	}

	void start(void)
	{
		in_->add_read_callback(this);
		in_->add_close_callback(this);
		in_->read();
	}

protected:
	// @override
	bool read_callback(char* data, int len)
	{
		printf("pipe_reader   <- %s", data);
		fflush(stdout);

		// transfer the data to thread_reader
		out_.get_ostream().write(data, len);
		return true;
	}

	// @override
	void close_callback(void)
	{
		printf("pipe_reader: being closed!\r\n");
		fflush(stdout);

		printf("pipe_reader: close pipe_out\r\n");
		out_.get_ostream().close();

		delete this;
	}

private:
	acl::aio_handle&  handle_;
	acl::aio_istream* in_;
	pipe_writer&      out_;

	~pipe_reader(void)
	{
		printf("%s: reader pipe will be deleted!\r\n", __FUNCTION__);
		if (--__aio_refer == 0) {
			printf("%s: stop aio engine now!\r\n", __FUNCTION__);
			handle_.stop();
		}
	}
};

//////////////////////////////////////////////////////////////////////////////

class thread_writer : public acl::thread
{
public:
	thread_writer(int fd)
	{
		out_ = new acl::socket_stream;
		out_->open(fd);
	}

	~thread_writer(void) {}

protected:
	// @override
	void* run(void)
	{
		acl::string buf("hello world!\r\n");
		for (int i = 0; i < 5; i++) {
			sleep(1);
			printf("\r\n");
			printf("thread_writer -> %s", buf.c_str());
			fflush(stdout);

			out_->write(buf);
		}

		printf("thread_writer: close out socket_stream\r\n");
		delete out_;
		return NULL;
	}

private:
	acl::socket_stream* out_;
};

class thread_reader : public acl::thread
{
public:
	thread_reader(int fd)
	{
		in_ = new acl::socket_stream;
		in_->open(fd);
	}

	~thread_reader(void) {}

protected:
	// @override
	void* run(void)
	{
		for (int i = 0; i < 5; i++) {
			acl::string buf;
			if (!in_->read(buf, false)) {
				printf("thread_reader: read over!\r\n");
				break;
			}
			printf("thread_reader <- %s", buf.c_str());
		}

		printf("thread_reader: close in socket_stream\r\n");
		delete in_;

		return NULL;
	}

private:
	acl::socket_stream* in_;
};

//////////////////////////////////////////////////////////////////////////////

class http_aio_client : public acl::http_aclient
{
public:
	http_aio_client(acl::aio_handle& handle, acl::sslbase_conf* ssl_conf,
		const char* host)
	: http_aclient(handle, ssl_conf)
	, host_(host)
	, debug_(false)
	, compressed_(false)
	, ws_mode_(false)
	{
		++__aio_refer;
	}

	~http_aio_client(void)
	{
		printf("delete http_aio_client!\r\n");
		if (--__aio_refer == 0) {
			printf("%s: stop aio engine now!\r\n", __FUNCTION__);
			handle_.stop();
		}
	}

	http_aio_client& enable_debug(bool on)
	{
		debug_ = on;
		return *this;
	}

	http_aio_client& enable_websocket(bool on)
	{
		ws_mode_ = on;
		return *this;
	}

protected:
	// @override
	void destroy(void)
	{
		printf("http_aio_client will be deleted!\r\n");
		fflush(stdout);

		delete this;
	}

	// @override
	bool on_connect(void)
	{
		printf("--------------- connect server ok ------------\r\n");
		fflush(stdout);

		if (ws_mode_) {
			printf(">>> begin ws_handshake\r\n");
			this->ws_handshake();
		} else {
			printf(">>> begin send http request\r\n");
			this->send_request(NULL, 0);
		}
		return true;
	}

	// @override
	void on_disconnect(void)
	{
		printf("disconnect from server\r\n");
		fflush(stdout);
	}

	// @override
	void on_ns_failed(void)
	{
		printf("dns lookup failed\r\n");
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

protected:
	// @override
	bool on_http_res_hdr(const acl::http_header& header)
	{
		acl::string buf;
		header.build_response(buf);

		compressed_ = header.is_transfer_gzip();

		printf("---------------response header-----------------\r\n");
		printf("[%s]\r\n", buf.c_str());
		fflush(stdout);

		return true;
	}

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
		this->ws_read_wait(0);
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
	bool ws_mode_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		" -s server_addr\r\n"
		" -H host[default: www.baidu.com]\r\n"
		" -L url[default: /]\r\n"
		" -D [if in debug mode, default: false]\r\n"
		" -c cocorrent\r\n"
		" -t connect_timeout[default: 5]\r\n"
		" -i rw_timeout[default: 5]\r\n"
		" -Z [enable_gzip, default: false]\r\n"
		" -U [enable_unzip response, default: false]\r\n"
		" -K [keep_alive, default: false]\r\n"
		" -S ssl_path[default: none]\n"
		" -N name_server[default: 8.8.8.8:53]\r\n"
		" -W [if using websocket, default: no]\r\n"
		, procname);
}

static void add_dns(std::vector<acl::string>& name_servers, const char* s)
{
	acl::string buf(s);
	const std::vector<acl::string>& tokens = buf.split2(",; \t");
	for (std::vector<acl::string>::const_iterator cit = tokens.begin();
		cit != tokens.end(); ++cit) {

		name_servers.push_back(*cit);
	}
}

int main(int argc, char* argv[])
{
	acl::sslbase_conf* ssl_conf = NULL;
	int  ch, conn_timeout = 5, rw_timeout = 5;
	std::vector<acl::string> name_servers;
	acl::string addr("127.0.0.1:80");
	acl::string host("www.baidu.com"), url("/"), ssl_path;
	bool enable_gzip = false, keep_alive = false, debug = false;
	bool ws_enable = false, enable_unzip = false;

	while ((ch = getopt(argc, argv, "hs:S:N:H:L:t:i:ZUKDW")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 's':
			addr = optarg;
			break;
		case 'S':
			ssl_path = optarg;
			break;
		case 'L':
			url = optarg;
			break;
		case 'N':
			add_dns(name_servers, optarg);
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
		case 'Z':
			enable_gzip = true;
			break;
		case 'U':
			enable_unzip = true;
			break;
		case 'K':
			keep_alive = true;
			break;
		case 'D':
			debug = true;
			break;
		case 'W':
			ws_enable = true;
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

	// 如果设置了 SSL 连接库，则启用 SSL 连接模式
	if (ssl_path.empty()) {
		/* do nothing */
	} else if (ssl_path.find("mbedtls") != NULL) {
		// 设置 libmbedtls 库全路径
		const std::vector<acl::string>& libs = ssl_path.split2("; \r");
		if (libs.size() == 3) {
			acl::mbedtls_conf::set_libpath(libs[0], libs[1], libs[2]);

			// 动态加载 libmbedtls_all.so 库
			acl::mbedtls_conf::load();

			// 创建全局 SSL 配置项
			ssl_conf = new acl::mbedtls_conf(false);
			printf(">>>use mbedtls<<<\r\n");
		}

	} else {
		// 设置 libpolarssl.so 库全路径
		acl::polarssl_conf::set_libpath(ssl_path);

		// 动态加载 libpolarssl.so 库
		acl::polarssl_conf::load();

		// 创建全局 SSL 配置项
		ssl_conf = new acl::polarssl_conf;
		printf(">>>use polarssl<<<\r\n");
	}

	// 定义 AIO 事件引擎
	acl::aio_handle handle(acl::ENGINE_KERNEL);

	int fds[2];

	//////////////////////////////////////////////////////////////////////

	int ret = acl_sane_socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
	if (ret < 0) {
		printf("acl_sane_socketpair error %s\r\n", acl::last_serror());
		return 1;
	}

	thread_reader* thread_in  = new thread_reader(fds[0]);
	pipe_writer*   pipe_out   = new pipe_writer(handle, fds[1]);

	thread_in->start();
	pipe_out->start();

	ret = acl_sane_socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
	if (ret < 0) {
		printf("acl_sane_socketpair error %s\r\n", acl::last_serror());
		return 1;
	}

	pipe_reader*   pipe_in    = new pipe_reader(handle, fds[0], *pipe_out);
	thread_writer* thread_out = new thread_writer(fds[1]);

	pipe_in->start();
	thread_out->start();

	//////////////////////////////////////////////////////////////////////

	for (std::vector<acl::string>::const_iterator cit = name_servers.begin();
		cit != name_servers.end(); ++cit) {

		// 设置 DNS 域名服务器地址
		handle.set_dns((*cit).c_str(), 5);
	}

	// 开始异步连接远程 WEB 服务器
	http_aio_client* conn = new http_aio_client(handle, ssl_conf, host);
	if (!conn->open(addr, conn_timeout, rw_timeout)) {
		printf("connect %s error\r\n", addr.c_str());
		fflush(stdout);

		delete conn;
		return 1;
	}

	(*conn).enable_debug(debug)		// 是否启用调试方式
		.enable_websocket(ws_enable);	// 是否启用 websocket
	conn->unzip_body(enable_unzip);		// 针对 HTTP 是否自动解压

	// 设置 HTTP 请求头，也可将此过程放在 conn->on_connect() 里
	acl::http_header& head = conn->request_header();
	head.set_url(url)
		.set_content_length(0)
		.set_host(host)
		.accept_gzip(enable_gzip)
		.set_keep_alive(keep_alive);

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
	delete ssl_conf;

	thread_out->wait(NULL);
	thread_in->wait(NULL);

	delete thread_out;
	delete thread_in;

	return 0;
}
