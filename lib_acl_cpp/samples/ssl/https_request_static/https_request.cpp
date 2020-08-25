#include "stdafx.h"
#include "https_request.h"

https_request::https_request(acl::sslbase_conf* ssl_conf, const char* addr,
	const char* host, const char* url)
: request_(addr)
, host_(host)
, url_(url)
, to_charset_("utf-8")
{
	printf("server addr: %s\r\n", addr);
	printf("host: %s\r\n", host);
	printf("url: %s\r\n", url);
	printf("\r\n");

	if (ssl_conf) {
		request_.set_ssl(ssl_conf);
	}
}

https_request::~https_request(void)
{
}

void* https_request::run(void)
{
	acl::http_header& hdr = request_.request_header();
	hdr.set_url(url_)
		.set_host(host_)
		.set_content_type("text/plain")
		.set_keep_alive(true);

	if (!request_.request(NULL, 0)) {
		printf("send request error\r\n");
		return NULL;
	}

	const char* ptr = request_.header_value("Content-Type");
	if (ptr == NULL || *ptr == 0) {
		printf("Content-Type empty!\r\n");
		return NULL;
	}

	acl::http_ctype ctype;
	ctype.parse(ptr);

	// 响应头数据类型的子类型
	const char* stype = ctype.get_stype();

	bool ret;

	if (stype == NULL) {
		ret = do_plain(request_);
	} else if (strcasecmp(stype, "xml") == 0) {
		ret = do_xml(request_);
	} else if (strcasecmp(stype, "json") == 0) {
		ret = do_json(request_);
	} else {
		ret = do_plain(request_);
	}

	if (ret) {
		printf("%s(%d): read ok!\r\n", __FILE__, __LINE__);
	} else {
		printf("read error\r\n");
	}

	return NULL;
}

bool https_request::do_plain(acl::http_request& req)
{
	acl::string body;
	if (!req.get_body(body, to_charset_)) {
		logger_error("get http body error");
		return false;
	}
	printf("plain body:\r\n(%s)\r\n", body.c_str());
	return true;
}

bool https_request::do_xml(acl::http_request& req)
{
	acl::xml1 body;
	if (req.get_body(body, to_charset_)) {
		logger_error("get http body error");
		return false;
	}

	printf(">>>xml body:\r\n[%s]\r\n", body.to_string());
	return true;
}

bool https_request::do_json(acl::http_request& req)
{
	acl::json body;
	if (!req.get_body(body, to_charset_)) {
		logger_error("get http body error");
		return false;
	}

	printf(">>>json body:\r\n[%s]\r\n", body.to_string().c_str());
	return true;
}
