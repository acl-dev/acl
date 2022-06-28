#include "stdafx.h"
#include "status/HttpClient.h"

HttpClient::HttpClient(const char* server_addr, const acl::string* buf)
: server_addr_(server_addr)
, buf_(buf)
, auto_free_(false)
{
}

HttpClient::~HttpClient()
{
}

void HttpClient::set_auto_free(bool on)
{
	auto_free_ = on;
}

// 基类虚函数

void* HttpClient::run()
{
	if (send() == false)
		logger_error("send to %s error", server_addr_.c_str());

	if (auto_free_)
		delete this;

	return NULL;
}

bool HttpClient::send()
{
	// 创建  HTTP 请求客户端
	acl::http_request req(server_addr_);
	acl::http_header& hdr = req.request_header();

	// 设置请求的 URL
	hdr.set_url("/");

	// 设置 HTTP 请求头的字段
	hdr.set_content_type("text/json; charset=gb2312");
	hdr.set_keep_alive(false);

	// 发送数据体
	if (req.request(buf_->c_str(), buf_->length()) == false)
	{
		logger_error("request to server error, addr: %s",
				server_addr_.c_str());
		return false;
	}

	int   status = req.http_status();
	if (status != 200)
	{
		logger_error("server(%s) return status: %d",
				server_addr_.c_str(), status);
		return false;
	}

	long long int length = req.body_length();
	if (length <= 0)
		return true;

	// 将响应数据体读完
	acl::string buf;
	if (req.get_body(buf) == false)
	{
		logger_error("read response body failed!");
		return false;
	}

	return true;
}
