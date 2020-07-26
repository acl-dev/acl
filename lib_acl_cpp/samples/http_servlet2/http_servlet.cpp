#include "stdafx.h"
#include "http_servlet.h"

http_servlet::http_servlet(acl::socket_stream* stream, acl::session* session)
	: acl::HttpServlet(stream, session)
{

}

http_servlet::~http_servlet(void)
{

}

bool http_servlet::doError(acl::HttpServletRequest&, acl::HttpServletResponse&)
{
	return false;
}

bool http_servlet::doUnknown(acl::HttpServletRequest&,
	acl::HttpServletResponse& res)
{
	res.setStatus(400);
	res.setContentType("text/html; charset=");
	// 发送 http 响应头
	if (res.sendHeader() == false)
		return false;
	// 发送 http 响应体
	acl::string buf("<root error='unkown request method' />\r\n");
	(void) res.getOutputStream().write(buf);
	return false;
}

bool http_servlet::doGet(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	return doPost(req, res);
}

bool http_servlet::doPost(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	bool keep_alive = req.isKeepAlive();

	res.setContentType("text/html; charset=utf-8")	// 设置响应字符集
		.setKeepAlive(keep_alive)		// 设置是否保持长连接
		.setChunkedTransferEncoding(true);	// chunked 传输模式

	// 发送 http 响应体
	acl::string buf("first line\r\nsecond line\r\nthird line\r\n\r\n");
	if (res.write(buf) == false)
	{
		logger_error("write error!");
		return false;
	}

	for (int i = 0; i < 10; i++)
	{
		acl::string buf("hello");
		if (res.write(buf) == false)
		{
			logger_error("write error!");
			return false;
		}
		if (res.write((acl::string)" ") == false)
		{
			logger_error("write error!");
			return false;
		}
		if (res.write((acl::string)"world") == false)
		{
			logger_error("write error!");
			return false;
		}
		if (res.write((acl::string)"\r\n") == false)
		{
			logger_error("write error!");
			return false;
		}
	}

	for (int j = 0; j < 10; j++)
	{
		for (int i = 0; i < 10; i++)
		{
			if (res.write((acl::string)"X") == false)
			{
				logger_error("write error!");
				return false;
			}
		}

		if (res.write((acl::string)"\r\n") == false)
		{
			logger_error("write error!");
			return false;
		}
	}

	// 最后一行不写 \r\n
	if (res.write((acl::string)"Bye") == false)
	{
		logger_error("write error!");
		return false;
	}

	return res.write(NULL, 0) && keep_alive;
}
