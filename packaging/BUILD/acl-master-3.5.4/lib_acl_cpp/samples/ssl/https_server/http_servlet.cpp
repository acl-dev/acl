#include "stdafx.h"
#include "http_servlet.h"

http_servlet::http_servlet(void)
{

}

http_servlet::~http_servlet(void)
{

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
	const char* mycookie = req.getCookieValue("mycookie");
	if (mycookie == NULL)
		res.addCookie("mycookie", "{xxx}");
	*/

	const char* param1 = req.getParameter("name1");
	const char* param2 = req.getParameter("name2");

	// 创建 xml 格式的数据体
	acl::xml1 body;
	body.get_root()
		.add_child("root", true)
			.add_child("params", true)
				.add_child("param", true)
					.add_attr("name1", param1 ? param1 : "null")
				.get_parent()
				.add_child("param", true)
					.add_attr("name2", param2 ? param2 : "null");
	acl::string buf;
	body.build_xml(buf);

	printf(">>>buf: %s\r\n", buf.c_str());

	bool keep_alive = req.isKeepAlive();

	res.setContentType("text/xml; charset=utf-8")	// 设置响应字符集
		.setKeepAlive(keep_alive)		// 设置是否保持长连接
		.setContentLength(buf.size())
		.setChunkedTransferEncoding(true);	// 采用 chunk 传输方式

	// 发送 http 响应体，因为设置了 chunk 传输模式，所以需要多调用一次
	// res.write 且两个参数均为 0 以表示 chunk 传输数据结束
	bool ret = res.write(buf) && res.write(NULL, 0) && keep_alive;
	printf(">>>ret: %s\r\n", ret ? "ok":"err");
	return ret;
}
