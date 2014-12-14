#include "stdafx.h"
#include "status/HttpClientRpc.h"

HttpClientRpc::HttpClientRpc(acl::string* buf, const char* server_addr)
: buf_(buf)
, server_addr_(server_addr)
{
}

HttpClientRpc::~HttpClientRpc()
{
	delete buf_;
}

/////////////////////////////////////////////////////////////////////////////
// 子线程中运行
void HttpClientRpc::rpc_run()
{
	// 创建  HTTP 请求客户端
	acl::http_request req(server_addr_);
	acl::http_header& hdr = req.request_header();
	hdr.set_url("/");
	// 设置 HTTP 请求头的字段
	hdr.set_content_type("text/json; charset=gb2312");

	// 发送数据体
	if (req.request(buf_->c_str(), buf_->length()) == false)
	{
		logger_error("request to server error, addr: %s",
			server_addr_.c_str());
		return;
	}

	int   status = req.http_status();
	if (status != 200)
	{
		logger_error("server(%s) return status: %d",
			server_addr_.c_str(), status);
		return;
	}

	long long int length = req.body_length();
	if (length <= 0)
		return;

	// 将响应数据体读完
	acl::string buf;
	if (req.get_body(buf) == false)
		logger_error("read response body failed!");
}

/////////////////////////////////////////////////////////////////////////////

// 主线程中运行
void HttpClientRpc::rpc_onover()
{
	delete this;
}
