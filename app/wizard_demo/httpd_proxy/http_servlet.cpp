#include "stdafx.h"
#include "fiber_transfer.h"
#include "http_servlet.h"

http_servlet::http_servlet(acl::socket_stream* stream, acl::session* session)
: acl::HttpServlet(stream, session)
{
	handlers_["/hello"] = &http_servlet::on_hello;
}

http_servlet::~http_servlet(void)
{
}

bool http_servlet::doError(request_t&, response_t& res)
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

bool http_servlet::doOther(request_t&, response_t& res, const char* method)
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

bool http_servlet::doGet(request_t& req, response_t& res)
{
	return doPost(req, res);
}

bool http_servlet::doPost(request_t& req, response_t& res)
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

	const char* path = req.getPathInfo();
	handler_t handler = path && *path ? handlers_[path] : NULL;
	return handler ? (this->*handler)(req, res) : on_default(req, res);
}

bool http_servlet::on_default(request_t& req, response_t& res)
{
	return on_hello(req, res);
}

bool http_servlet::on_hello(request_t& req, response_t& res)
{
	res.setContentType("text/html; charset=utf-8")	// 设置响应字符集
		.setKeepAlive(req.isKeepAlive())	// 设置是否保持长连接
		.setContentEncoding(true)		// 自动支持压缩传输
		.setChunkedTransferEncoding(true);	// 采用 chunk 传输方式

	acl::string buf;
	buf.format("<html><body>xxxxxxx<br>\r\n");
	if (res.write(buf) == false) {
		printf("write error\r\n");
		return false;
	}

	acl::json* json = req.getJson();
	if (json == NULL) {
		printf("json null\r\n");
	} else {
		printf("json is [%s]\r\n", json->to_string().c_str());
	}
	for (size_t i = 0; i < 1; i++)
	{
		buf.format("hello world=%d<br>\r\n", (int) i);
		if (res.write(buf) == false) {
			printf("write error\r\n");
			return false;
		}

		if (i % 10000 == 0)
		{
			sleep(1);
			printf("i=%d\n", (int) i);
		}
	}

	buf = "</body></html><br>\r\n";
	printf("write ok\n");

	return res.write(buf) && res.write(NULL, 0);
}

bool http_servlet::doConnect(request_t& req, response_t& res)
{
	// CONNECT 127.0.0.1:22 HTTP/1.0
	// HTTP/1.1 200 Connection Established

	const char* host = req.getRemoteHost();
	if (host == NULL || *host == 0) {
		printf("getRemoteHost null\r\n");
		return false;
	}
	printf("remote host=%s\r\n", host);

	acl::socket_stream peer;
	if (peer.open(host, 0, 0) == false) {
		printf("connect %s error %s\r\n", host, acl::last_serror());
		return false;
	}

	//const char* ok = "HTTP/1.1 200 Connection Established\r\n";
	//acl::ostream& out = res.getOutputStream();

	const char* ok = "";
	res.setContentLength(0);
	if (res.write(ok) == false) {
		return false;
	}

	acl::socket_stream& local = req.getSocketStream();
	doProxy(local, peer);
	return false;
}

bool http_servlet::doProxy(acl::socket_stream& local, acl::socket_stream& peer)
{
	fiber_transfer fiber_local(local, peer);
	fiber_transfer fiber_peer(peer, local);

	fiber_local.set_peer(fiber_peer);
	fiber_peer.set_peer(fiber_local);

	fiber_local.start();
	fiber_peer.start();

	fiber_local.wait();
	fiber_peer.wait();

	printf("doProxy finished, local fd=%d, peer fd=%d\r\n",
		fiber_local.get_input().sock_handle(),
		fiber_local.get_output().sock_handle());
	return true;
}
