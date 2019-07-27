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

// 鍩虹被铏氬嚱鏁�

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
	// 鍒涘缓  HTTP 璇锋眰瀹㈡埛绔�
	acl::http_request req(server_addr_);
	acl::http_header& hdr = req.request_header();

	// 璁剧疆璇锋眰鐨� URL
	hdr.set_url("/");

	// 璁剧疆 HTTP 璇锋眰澶寸殑瀛楁
	hdr.set_content_type("text/json; charset=gb2312");
	hdr.set_keep_alive(false);

	// 鍙戦€佹暟鎹綋
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

	// 灏嗗搷搴旀暟鎹綋璇诲畬
	acl::string buf;
	if (req.get_body(buf) == false)
	{
		logger_error("read response body failed!");
		return false;
	}

	return true;
}
