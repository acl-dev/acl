#include "stdafx.h"
#include "https_client.h"
#include "http_servlet.h"

http_servlet::http_servlet(acl::ostream& out, acl::sslbase_conf* conf)
: handled_(false)
, out_(out)
, client_ssl_conf_(conf)
{

}

http_servlet::~http_servlet(void)
{

}

void http_servlet::logger_request(acl::HttpServletRequest& req)
{
	acl::string req_hdr;
	acl::http_client* client = req.getClient();
	client->sprint_header(req_hdr, NULL);
	out_.format("\r\n>>>request header<<<\r\n");
	out_.write(req_hdr);
	out_.format("\r\n");
}

bool http_servlet::doError(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
//	if (handled_)
		return false;

	out_.format(">>> request method: doError <<<\r\n");
	logger_request(req);

	res.setStatus(400);
	res.setContentType("text/html; charset=");
	// 发送 http 响应体
	acl::string buf("<root error='error request' />\r\n");
	(void) res.getOutputStream().write(buf);
	return false;
}

bool http_servlet::doUnknown(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	out_.format(">>> request method: doUnknown <<<\r\n");
	logger_request(req);

	res.setStatus(400);
	res.setContentType("text/html; charset=");
	// 发送 http 响应体
	acl::string buf("<root error='unkown request method' />\r\n");
	(void) res.getOutputStream().write(buf);
	return false;
}

bool http_servlet::doPut(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	handled_ = true;
	out_.format(">>> request method: PUT <<<\r\n");
	logger_request(req);
	return doPost(req, res);
}

bool http_servlet::doConnect(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	handled_ = true;
	out_.format(">>> request method: CONNECT <<<\r\n");
	logger_request(req);
	return doPost(req, res);
}

bool http_servlet::doDelete(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	handled_ = true;
	out_.format(">>> request method: DELETE <<<\r\n");
	logger_request(req);
	return doPost(req, res);
}

bool http_servlet::doHead(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	handled_ = true;
	out_.format(">>> request method: HEAD <<<\r\n");
	logger_request(req);
	return doPost(req, res);
}

bool http_servlet::doOptions(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	handled_ = true;
	out_.format(">>> request method: OPTIONS <<<\r\n");
	return doPost(req, res);
}

bool http_servlet::doPropfind(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	handled_ = true;
	out_.format(">>> request method: PROPFIND <<<\r\n");
	return doPost(req, res);
}

bool http_servlet::doOther(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res, const char* method)
{
	handled_ = true;
	out_.format(">>> request method: %s <<<\r\n", method);
	return doPost(req, res);
}

bool http_servlet::doGet(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	handled_ = true;
	out_.format(">>> request method: GET <<<\r\n");
	return doPost(req, res);
}

bool http_servlet::doPost(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	handled_ = true;
	acl::http_client* conn = req.getClient();
	conn->header_disable("Accept-Encoding");
	logger_request(req);

	// 生成完整的 url，以备下面使用
	const char* host = req.getRemoteHost();
	const char* uri = req.getRequestUri();
	if (host == NULL || *host == 0)
		host = req.getLocalAddr();

	url_.format("http://%s%s", host, uri ? uri : "/");
	out_.format(">>> request url: %s\r\n", url_.c_str());

	https_client client(out_, client_ssl_conf_);
	return client.http_request(req, res);
}
