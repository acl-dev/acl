#include "stdafx.h"
#include "tcp_transfer.h"
#include "http_transfer.h"
#include "http_servlet.h"

http_servlet::http_servlet(acl::socket_stream* stream, acl::session* session,
	int port /* 80 */)
: acl::HttpServlet(stream, session)
, port_(port)
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
	const char* path = req.getPathInfo();
	handler_t handler = path && *path ? handlers_[path] : NULL;
	return handler ? (this->*handler)(req, res) : transfer_get(req, res);
}

bool http_servlet::doPost(request_t& req, response_t& res)
{
	const char* path = req.getPathInfo();
	handler_t handler = path && *path ? handlers_[path] : NULL;
	return handler ? (this->*handler)(req, res) : transfer_post(req, res);
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
	for (size_t i = 0; i < 1; i++) {
		buf.format("hello world=%d<br>\r\n", (int) i);
		if (res.write(buf) == false) {
			printf("write error\r\n");
			return false;
		}

		if (i % 10000 == 0) {
			sleep(1);
			printf("i=%d\n", (int) i);
		}
	}

	buf = "</body></html><br>\r\n";
	printf("write ok\n");

	return res.write(buf) && res.write(NULL, 0);
}

bool http_servlet::transfer_get(request_t& req, response_t& res)
{
	http_transfer* fiber_peer = new
		http_transfer(acl::HTTP_METHOD_GET, req, res, port_);
	fiber_peer->start();

	bool keep_alive;
	fiber_peer->wait(&keep_alive);

	delete fiber_peer;
	return keep_alive && req.isKeepAlive();
}

bool http_servlet::transfer_post(request_t& req, response_t& res)
{
	http_transfer* fiber_peer = new
		http_transfer(acl::HTTP_METHOD_POST, req, res, port_);
	fiber_peer->start();

	bool keep_alive;
	fiber_peer->wait(&keep_alive);

	delete fiber_peer;
	printf("transfer_post finished\r\n");
	return keep_alive && req.isKeepAlive();
}

bool http_servlet::doConnect(request_t& req, response_t&)
{
	// CONNECT 127.0.0.1:22 HTTP/1.0
	// HTTP/1.1 200 Connection Established

	const char* phost = req.getRemoteHost();
	if (phost == NULL || *phost == 0) {
		logger_error("getRemoteHost null");
		return false;
	}

	acl::string host;
	const char* port = strrchr(phost, ':');
	if (port == NULL) {
		host.format("%s|80", phost);
	} else if (*(port + 1) == 0) {
		host.format("%s80", phost);
	} else {
		host = phost;
	}

	printf("remote host=%s, current fiber=%p\r\n", host.c_str(), acl_fiber_running());

	acl::socket_stream* peer = new acl::socket_stream;
	if (!peer->open(host, 5, 5, acl::time_unit_s)) {
		logger_error("connect %s error %s", host.c_str(), acl::last_serror());
		delete peer;
		return false;
	}
	printf("connect %s ok, fd=%d\r\n", host.c_str(), peer->sock_handle());

#define	USE_REFER

#ifdef	USE_REFER
	acl::socket_stream* local = &req.getSocketStream();
#else
	acl::socket_stream* local = new acl::socket_stream;
	local->open(req.getSocketStream().sock_handle());
#endif

#if 0
	const char* ok = "";
	res.setContentLength(0);
	if (!res.write(ok, 1)) {
		logger_error("write connect header error");
		return false;
	}
#else
	const char* ok = "HTTP/1.1 200 Connection Established\r\n\r\n";
	size_t n = strlen(ok);

	if (local->write(ok, n) != (int) n) {
		logger_error("write connect response error");
		delete peer;

		local->unbind_sock();
		delete local;
		return false;
	}
#endif

	transfer_tcp(*local, *peer);

#ifndef	USE_REFER
	int fd = local->unbind_sock();
	if (fd == -1) {
		acl::socket_stream& ss = req.getSocketStream();
		logger_warn("The socket=%d has been closed before!",
			ss.sock_handle());
		ss.unbind_sock();
	}
	delete local;
#endif
	delete peer;
	return false;
}

bool http_servlet::transfer_tcp(acl::socket_stream& local,
	acl::socket_stream& peer)
{
	local.set_rw_timeout(20);
	peer.set_rw_timeout(20);

	tcp_transfer* fiber_local = new
		tcp_transfer(acl_fiber_running(), local, peer, false);
	tcp_transfer* fiber_peer = new
		tcp_transfer(acl_fiber_running(), peer, local, false);

	fiber_local->set_peer(fiber_peer);
	fiber_local->set_local(true);

	fiber_peer->set_peer(fiber_local);
	fiber_peer->set_local(false);

	fiber_peer->start();
	fiber_local->start();

	//int fd_local = local.sock_handle();
	//int fd_peer = peer.sock_handle();

	//printf("wait local fiber, local fd=%d, peer fd=%d\r\n", fd_local, fd_peer);
	fiber_local->wait();
	//printf("local fiber done, local fd=%d, peer fd=%d\r\n", fd_local, fd_peer);

	//printf("wait peer fiber, local fd=%d, peer fd=%d\r\n", fd_local, fd_peer);
	fiber_peer->wait();
	//printf("peer fiber done, local fd=%d, peer fd=%d\r\n", fd_local, fd_peer);

	//printf("transfer_tcp finished, local fd=%d, %d, peer fd=%d, %d\r\n",
	//	fiber_local.get_input().sock_handle(), fd_local,
	//	fiber_local.get_output().sock_handle(), fd_peer);

	delete fiber_peer;
	delete fiber_local;
	return true;
}
