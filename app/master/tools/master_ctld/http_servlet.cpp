#include "stdafx.h"
#include "daemon/version.h"
#include "action/action.h"
#include "master_ctld_version.h"
#include "http_servlet.h"

http_servlet::http_servlet(acl::socket_stream* stream, acl::session* session,
	const char* addr, const char* path)
: acl::HttpServlet(stream, session)
, addr_(addr)
, conf_(path)
{
}

http_servlet::~http_servlet(void)
{
}

bool http_servlet::replyf(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res, int status, const char* fmt, ...)
{
	acl::string buf;
	va_list ap;
	va_start(ap, fmt);
	buf.vformat(fmt, ap);
	va_end(ap);

	return reply(req, res, status, buf);
}

bool http_servlet::reply(acl::HttpServletRequest&,
	acl::HttpServletResponse& res, int status, const acl::string& buf)
{
	res.setStatus(status)
		.setContentType("text/plain; charset=utf-8")
		.setContentLength(buf.size());

	return res.write(buf, buf.size()) && res.write(NULL, 0);
}

bool http_servlet::reply_json(acl::HttpServletRequest&,
	acl::HttpServletResponse& res, int status, const acl::string& data)
{
	res.setStatus(status)
		.setContentType("text/json; charset=utf-8")
		.setContentLength(data.size());

	return res.write(data, data.size()) && res.write(NULL, 0);
}

bool http_servlet::doError(acl::HttpServletRequest&,
	acl::HttpServletResponse& res)
{
	res.setStatus(400);
	res.setContentType("text/xml; charset=utf-8");

	// 发送 http 响应体
	acl::string buf;
	buf.format("<root error='some error happened!' />\r\n");
	res.write(buf);
	res.write(NULL, 0);
	return false;
}

bool http_servlet::doOther(acl::HttpServletRequest&,
	acl::HttpServletResponse& res, const char* method)
{
	res.setStatus(400);
	res.setContentType("text/xml; charset=utf-8");
	// 发送 http 响应体
	acl::string buf;
	buf.format("<root error='unkown request method %s' />\r\n", method);
	res.write(buf);
	res.write(NULL, 0);
	return false;
}

bool http_servlet::doGet(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	res.setContentType("text/plain")
		.setKeepAlive(req.isKeepAlive());

	acl::string body;
	body.format("%s version: %s; %s %s %s\r\n",
		MASTER_NAME, MASTER_VERSION,
		MASTER_CTLD_CMD, MASTER_CTLD_VERSION, MASTER_CTLD_DATE);

	return reply(req, res, 200, body);
}

bool http_servlet::doPost(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	// 如果需要 http session 控制，请打开下面注释，且需要保证
	// 在 master_service.cpp 的函数 thread_on_read 中设置的
	// memcached 服务正常工作
	/*
	const char* sid = req.getSession().getAttribute("sid");
	if (*sid == 0)
		req.getSession().setAttribute("sid", "xxxxxx");
	sid = req.getSession().getAttribute("sid");
	*/

	// 如果需要取得浏览器 cookie 请打开下面注释
	/*
	
	*/

	res.setContentType("text/xml; charset=utf-8")	// 设置响应字符集
		.setKeepAlive(req.isKeepAlive())	// 设置是否保持长连接
		.setContentEncoding(true)		// 自动支持压缩传输
		.setChunkedTransferEncoding(false);	// chunk 传输方式

	const char* cmd = req.getParameter("cmd");
	if (cmd == NULL || *cmd == 0)
	{
		logger_error("cmd not found");
		return replyf(req, res, 400, "%s", "no cmd");
	}

#define EQ	!strcasecmp

	acl::string buf;
	commands_action action(addr_, req, res, cmd);
	action.set_conf(conf_);
	int status = action.run(buf);
	return reply_json(req, res, status, buf);
}
