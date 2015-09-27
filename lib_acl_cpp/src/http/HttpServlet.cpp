#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/session/memcache_session.hpp"
#include "acl_cpp/http/http_header.hpp"
#include "acl_cpp/http/HttpSession.hpp"
#include "acl_cpp/http/HttpServletRequest.hpp"
#include "acl_cpp/http/HttpServletResponse.hpp"
#include "acl_cpp/http/HttpServlet.hpp"

namespace acl
{

HttpServlet::HttpServlet(socket_stream* stream, session* session)
: stream_(stream)
{
	init();

	if (session == NULL)
	{
		session_ = NEW memcache_session("127.0.0.1");
		session_ptr_ = session_;
	}
	else
	{
		session_ = session;
		session_ptr_ = NULL;
	}
}

HttpServlet::HttpServlet(socket_stream* stream,
	const char* memcache_addr /* = "127.0.0.1:11211" */)
: stream_(stream)
{
	init();

	session_ = NEW memcache_session(memcache_addr);
	session_ptr_ = session_;
}

HttpServlet::HttpServlet()
{
	init();

	stream_ = NULL;
	session_ = NULL;
	session_ptr_ = NULL;
}

void HttpServlet::init()
{
	first_ = true;
	local_charset_[0] = 0;
	rw_timeout_ = 60;
	parse_body_enable_ = true;
	parse_body_limit_ = 0;
}

HttpServlet::~HttpServlet(void)
{
	delete session_ptr_;
}

#define COPY(x, y) ACL_SAFE_STRNCPY((x), (y), sizeof((x)))

HttpServlet& HttpServlet::setLocalCharset(const char* charset)
{
	if (charset && *charset)
		COPY(local_charset_, charset);
	else
		local_charset_[0] =0;
	return *this;
}

HttpServlet& HttpServlet::setRwTimeout(int rw_timeout)
{
	rw_timeout_ = rw_timeout;
	return *this;
}

HttpServlet& HttpServlet::setParseBody(bool on)
{
	parse_body_enable_ = on;
	return *this;
}

HttpServlet& HttpServlet::setParseBodyLimit(int length)
{
	if (length > 0)
		parse_body_limit_ = length;
	return *this;
}

bool HttpServlet::doRun()
{
	socket_stream* in;
	socket_stream* out;
	bool cgi_mode;

	bool first = first_;
	if (first_)
		first_ = false;

	if (stream_ == NULL)
	{
		// 数据流为空，则当 CGI 模式处理，将标准输入输出
		// 作为数据流
		in = NEW socket_stream();
		in->open(ACL_VSTREAM_IN);

		out = NEW socket_stream();
		out->open(ACL_VSTREAM_OUT);
		cgi_mode = true;
	}
	else
	{
		in = out = stream_;
		cgi_mode = false;
	}

	// req/res 采用栈变量，减少内存分配次数

	HttpServletResponse res(*out);
	HttpServletRequest req(res, *session_, *in, local_charset_,
		parse_body_enable_, parse_body_limit_);

	// 设置 HttpServletRequest 对象
	res.setHttpServletRequest(&req);

	if (rw_timeout_ >= 0)
		req.setRwTimeout(rw_timeout_);

	res.setCgiMode(cgi_mode);

	string method_s(32);
	http_method_t method = req.getMethod(&method_s);

	// 根据请求的值自动设定是否需要保持长连接
	if (!cgi_mode)
		res.setKeepAlive(req.isKeepAlive());

	bool  ret;

	switch (method)
	{
	case HTTP_METHOD_GET:
		ret = doGet(req, res);
		break;
	case HTTP_METHOD_POST:
		ret = doPost(req, res);
		break;
	case HTTP_METHOD_PUT:
		ret = doPut(req, res);
		break;
	case HTTP_METHOD_CONNECT:
		ret = doConnect(req, res);
		break;
	case HTTP_METHOD_PURGE:
		ret = doPurge(req, res);
		break;
	case HTTP_METHOD_DELETE:
		ret = doDelete(req, res);
		break;
	case  HTTP_METHOD_HEAD:
		ret = doHead(req, res);
		break;
	case HTTP_METHOD_OPTION:
		ret = doOptions(req, res);
		break;
	case HTTP_METHOD_PROPFIND:
		ret = doPropfind(req, res);
		break;
	case HTTP_METHOD_OTHER:
		ret = doOther(req, res, method_s.c_str());
		break;
	default:
		ret = false; // 有可能是IO失败或未知方法
		if (req.getLastError() == HTTP_REQ_ERR_METHOD)
			doUnknown(req, res);
		else if (first)
			doError(req, res);
		break;
	}

	if (in != out)
	{
		// 如果是标准输入输出流，则需要先将数据流与标准输入输出解绑，
		// 然后才能释放数据流对象，数据流内部会自动判断流句柄合法性
		// 这样可以保证与客户端保持长连接
		in->unbind();
		out->unbind();
		delete in;
		delete out;
	}

	// 返回给上层调用者：true 表示继续保持长连接，否则表示需断开连接
	return ret && req.isKeepAlive()
		&& res.getHttpHeader().get_keep_alive();
}

bool HttpServlet::doRun(session& session, socket_stream* stream /* = NULL */)
{
	stream_ = stream;
	session_ = &session;
	return doRun();
}

bool HttpServlet::doRun(const char* memcached_addr, socket_stream* stream)
{
	memcache_session session(memcached_addr);
	return doRun(session, stream);
}

} // namespace acl
