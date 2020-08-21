#include "stdafx.h"
#include "http_service.h"
#include "http_servlet.h"

http_servlet::http_servlet(http_service& service, acl::socket_stream* conn,
	acl::session* session)
: acl::HttpServlet(conn, session), service_(service)
{
}

http_servlet::~http_servlet(void) {}

bool http_servlet::doGet(HttpRequest& req, HttpResponse& res)
{
	return doService(http_handler_get, req, res);
}

bool http_servlet::doPost(HttpRequest& req, HttpResponse& res)
{
	return doService(http_handler_post, req, res);
}

bool http_servlet::doHead(HttpRequest& req, HttpResponse& res)
{
	return doService(http_handler_head, req, res);
}

bool http_servlet::doPut(HttpRequest& req, HttpResponse& res)
{
	return doService(http_handler_put, req, res);
}

bool http_servlet::doPatch(HttpRequest& req, HttpResponse& res)
{
	return doService(http_handler_patch, req, res);
}

bool http_servlet::doConnect(HttpRequest& req, HttpResponse& res)
{
	return doService(http_handler_connect, req, res);
}

bool http_servlet::doPurge(HttpRequest& req, HttpResponse& res)
{
	return doService(http_handler_purge, req, res);
}

bool http_servlet::doDelete(HttpRequest& req, HttpResponse& res)
{
	return doService(http_handler_delete, req, res);
}

bool http_servlet::doOptions(HttpRequest& req, HttpResponse& res)
{
	return doService(http_handler_options, req, res);
}

bool http_servlet::doProfind(HttpRequest& req, HttpResponse& res)
{
	return doService(http_handler_profind, req, res);
}

bool http_servlet::doWebsocket(HttpRequest& req, HttpResponse& res)
{
	return doService(http_handler_websocket, req, res);
}

bool http_servlet::doUnknown(HttpRequest& req, HttpResponse& res)
{
	return doService(http_handler_unknown, req, res);
}

bool http_servlet::doError(HttpRequest& req, HttpResponse& res)
{
	return doService(http_handler_error, req, res);
}

bool http_servlet::doOther(HttpRequest&, HttpResponse& res, const char* method)
{
	res.setStatus(400);
	res.setContentType("text/xml; charset=utf-8");
	// 发送 http 响应体
	acl::string buf;
	buf.format("<root error='unkown method %s' />\r\n", method);
	res.write(buf);
	res.write(NULL, 0);
	return false;
}

bool http_servlet::doService(int type, HttpRequest& req, HttpResponse& res)
{
	// 如果需要 http session 控制，请打开下面注释，且需要保证
	// 在 master_service.cpp 的函数 thread_on_read 中设置的
	// memcached 或 redis 服务正常
	/*
	const char* sid = req.getSession().getAttribute("sid");
	if (*sid == 0) {
		req.getSession().setAttribute("sid", "xxxxxx");
	}
	sid = req.getSession().getAttribute("sid");
	*/

	// 如果需要取得浏览器 cookie 请打开下面注释
	/*
	$<GET_COOKIES> 
	*/

	return service_.doService(type, req, res);
}
