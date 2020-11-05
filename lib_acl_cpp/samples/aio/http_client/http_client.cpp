#include "stdafx.h"
#include "http_stream.h"
#include "http_client.h"

http_client::http_client(acl::aio_handle& handle)
: handle_(handle)
, refered_(0)
, conn_timeout_(5)
, rw_timeout_(5)
, debug_(false)
, redirect_limit_(5)
, redirect_count_(0)
, url_("/")
, keep_alive_(true)
, compressed_(false)
{
}

http_client::~http_client(void)
{
}

http_client& http_client::set_addr(const char* addr)
{
	addr_ = addr;
	return *this;
}

http_client& http_client::set_timeout(int conn_timeout, int rw_timeout)
{
	conn_timeout_ = conn_timeout;
	rw_timeout_   = rw_timeout;
	return *this;
}

http_client& http_client::set_debug(bool on)
{
	debug_ = on;
	return *this;
}

http_client& http_client::set_redirect_limit(int max)
{
	redirect_limit_ = max;
	return *this;
}

http_client& http_client::set_url(const char* url)
{
	url_ = url;
	return *this;
}

http_client& http_client::set_host(const char* host)
{
	host_ = host;
	return *this;
}

http_client& http_client::set_keep_alive(bool yes)
{
	keep_alive_ = yes;
	return *this;
}

bool http_client::open(void)
{
	http_stream* conn = new http_stream(handle_, *this);

	conn->unzip_body(true);

	acl::http_header& hdr = conn->request_header();
	hdr.set_url(url_)
		.set_host(host_)
		.accept_gzip(true)
		.set_keep_alive(keep_alive_);

	acl::string buf;
	hdr.build_request(buf);
	printf("---------------request header-----------------\r\n");
	printf("[%s]\r\n", buf.c_str());

	if (!conn->open(addr_, conn_timeout_, rw_timeout_)) {
		delete conn;
		return false;
	}

	++__aio_refer;

	// 本对象的引用计数递增
	refered_++;
	return true;
}

bool http_client::redirect(const char* url)
{
	char domain[256];
	unsigned short port;
	if (!acl::http_utils::get_addr(url, domain, sizeof(domain), &port)) {
		printf("invalid url=%s\r\n", url);
		return false;
	}

	printf("\r\nredirect to url=%s\r\n\r\n", url);

	const char http_pref[] = "http://", https_pref[] = "https://";

	if (!strncasecmp(url, http_pref, sizeof(http_pref) - 1)) {
		url += sizeof(http_pref) - 1;
	} else if (strncasecmp(url, https_pref, sizeof(https_pref) - 1)) {
		url += sizeof(https_pref) - 1;
	}

	const char* slash = strchr(url, '/');
	if (slash == NULL) {
		url = "/";
	} else {
		url = slash;
	}

	acl::string addr;
	addr.format("%s|%d", domain, port);

	set_addr(addr);
	set_url(url);
	set_host(domain);

	if (open()) {
		return true;
	}

	delete this;
	return false;
}

bool http_client::start(void)
{
	if (open()) {
		return true;
	}

	delete this;
	return false;
}

void http_client::on_destroy(http_stream* conn)
{
	printf("http_stream will be deleted!\r\n");
	delete conn;
	__destroy++;

	if (--__aio_refer == 0) {
		printf("%s: stop aio engine now!\r\n", __FUNCTION__);
		handle_.stop();
	}

	if (--refered_ == 0) {
		printf("============== delete http_client =============\r\n");
		delete this;
	}
}

void http_client::on_connect(http_stream& conn)
{
	printf("--------------- connect server ok ------------\r\n");
	acl::string addr;
	if (conn.get_ns_addr(addr)) {
		printf(">>>ns server: %s\r\n", addr.c_str());
	}
	__connect_ok++;
}

void http_client::on_disconnect(http_stream&)
{
	printf("disconnect from server\r\n");
	__disconnect++;
}

void http_client::on_ns_failed(http_stream&)
{
	printf("dns lookup failed\r\n");
	__ns_failed++;
}

void http_client::on_connect_timeout(http_stream&)
{
	printf("connect timeout\r\n");
	__connect_timeout++;
}

void http_client::on_connect_failed(http_stream&)
{
	printf("connect failed\r\n");
	__connect_failed++;
}

bool http_client::on_read_timeout(http_stream&)
{
	printf("read timeout\r\n");
	__read_timeout++;
	return false;
}

bool http_client::on_http_res_hdr(http_stream&, const acl::http_header& header)
{
	acl::string buf;
	header.build_response(buf);

	compressed_ = header.is_transfer_gzip();
	__header_ok++;

	int http_status = header.get_status();

	if (debug_) {
		printf("-----------%s: response header(status=%d)----\r\n",
			__FUNCTION__, http_status);
		printf("[%s]\r\n", buf.c_str());
	}

	if (http_status == 301 || http_status == 302) {
		const char* location = header.get_entry("Location");
		if (location == NULL || *location == 0) {
			printf("Location null\r\n");
			return false;
		}

		if (++redirect_count_ > redirect_limit_) {
			printf("\r\nTOO MANY redirect!(%d > %d)\r\n\r\n",
				redirect_count_, redirect_limit_);
			return false;
		}

		redirect(location);
		// 返回 false 以使当前连接关闭
		return false;
	}

	return true;
}

bool http_client::on_http_res_body(http_stream& conn, char* data, size_t dlen)
{
	if (!debug_) {
		return true;
	}

	bool ret = conn.is_unzip_body();
	printf(">>>read body: %ld, unzip_body: %s\r\n",
		(long) dlen, ret ? "yes" : "no");

	(void) write(1, data, dlen);
	return true;
}

bool http_client::on_http_res_finish(http_stream&, bool success)
{
	printf("\r\n---------------response over----------------\r\n");
	printf("http finish: keep_alive=%s, success=%s\r\n",
		keep_alive_ ? "true" : "false", success ? "ok" : "failed");
	__success++;

	return keep_alive_;
}
