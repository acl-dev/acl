#include "stdafx.h"
#include "https_request.h"

https_request::https_request(const char* addr, acl::polarssl_conf* ssl_conf)
	: request_(addr)
	, to_charset_("utf-8")
{
	if (ssl_conf)
		request_.set_ssl(ssl_conf);
}

https_request::~https_request(void)
{
}

void* https_request::run(void)
{
	acl::http_header& hdr = request_.request_header();
	hdr.set_url("/").set_content_type("text/plain").set_keep_alive(true);
	hdr.add_param("name1", "name1");

	if (request_.request(NULL, 0) == false)
	{
		printf("send request error\r\n");
		return NULL;
	}

	const char* ptr = request_.header_value("Content-Type");
	if (ptr == NULL || *ptr == 0)
	{
		printf("Content-Type empty!\r\n");
		return NULL;
	}

	acl::http_ctype ctype;
	ctype.parse(ptr);

	// 响应头数据类型的子类型
	const char* stype = ctype.get_stype();

	bool ret;

	if (stype == NULL)
		ret = do_plain(request_);
	else if (strcasecmp(stype, "xml") == 0)
		ret = do_xml(request_);
	else if (strcasecmp(stype, "json") == 0)
		ret = do_json(request_);
	else
		ret = do_plain(request_);

	if (ret == true)
		printf("read ok!\r\n");
	else
	{
		printf("read error\r\n");
		return NULL;
	}

	return NULL;
}

bool https_request::do_plain(acl::http_request& req)
{
	acl::string body;
	if (req.get_body(body, to_charset_) == false)
	{
		logger_error("get http body error");
		return false;
	}
	printf("body:\r\n(%s)\r\n", body.c_str());
	return true;
}

bool https_request::do_xml(acl::http_request& req)
{
	acl::xml1 body;
	if (req.get_body(body, to_charset_) == false)
	{
		logger_error("get http body error");
		return false;
	}

	printf(">>>xml: [%s]\r\n", body.to_string());

	acl::xml_node* node = body.first_node();
	while (node)
	{
		const char* tag = node->tag_name();
		const char* name1 = node->attr_value("name1");
		const char* name2 = node->attr_value("name2");
		printf(">>tag: %s, name1: %s, name2: %s\r\n",
			tag ? tag : "null",
			name1 ? name1 : "null",
			name2 ? name2 : "null");
		node = body.next_node();
	}
	return true;
}

bool https_request::do_json(acl::http_request& req)
{
	acl::json body;
	if (req.get_body(body, to_charset_) == false)
	{
		logger_error("get http body error");
		return false;
	}

	acl::json_node* node = body.first_node();
	while (node)
	{
		if (node->tag_name())
		{
			printf("tag: %s", node->tag_name());
			if (node->get_text())
				printf(", value: %s\r\n", node->get_text());
			else
				printf("\r\n");
		}
		node = body.next_node();
	}
	return true;
}
