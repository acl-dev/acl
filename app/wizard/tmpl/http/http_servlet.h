#pragma once

class http_servlet : public acl::HttpServlet
{
public:
	http_servlet(http_handlers_t* handlers, acl::socket_stream* conn,
		acl::session* session)
	: acl::HttpServlet(conn, session), handlers_(handlers) {}

	~http_servlet(void) {}

protected:
	// @override
	bool doGet(HttpRequest& req, HttpResponse& res)
	{
		return doService(http_handler_get, req, res);
	}

	// @override
	bool doPost(HttpRequest& req, HttpResponse& res)
	{
		return doService(http_handler_post, req, res);
	}

	// override
	bool doHead(HttpRequest& req, HttpResponse& res)
	{
		return doService(http_handler_head, req, res);
	}

	// override
	bool doPut(HttpRequest& req, HttpResponse& res)
	{
		return doService(http_handler_put, req, res);
	}

	// override
	bool doPatch(HttpRequest& req, HttpResponse& res)
	{
		return doService(http_handler_patch, req, res);
	}

	// override
	bool doConnect(HttpRequest& req, HttpResponse& res)
	{
		return doService(http_handler_connect, req, res);
	}

	// override
	bool doPurge(HttpRequest& req, HttpResponse& res)
	{
		return doService(http_handler_purge, req, res);
	}

	// override
	bool doDelete(HttpRequest& req, HttpResponse& res)
	{
		return doService(http_handler_delete, req, res);
	}

	// override
	bool doOptions(HttpRequest& req, HttpResponse& res)
	{
		return doService(http_handler_options, req, res);
	}

	// override
	bool doProfind(HttpRequest& req, HttpResponse& res)
	{
		return doService(http_handler_profind, req, res);
	}

	// override
	bool doWebsocket(HttpRequest& req, HttpResponse& res)
	{
		return doService(http_handler_websocket, req, res);
	}

	// override
	bool doUnknown(HttpRequest& req, HttpResponse& res)
	{
		return doService(http_handler_unknown, req, res);
	}

	// @override
	bool doError(HttpRequest& req, HttpResponse& res)
	{
		return doService(http_handler_error, req, res);
	}

	// @override
	bool doOther(HttpRequest&, HttpResponse& res, const char* method)
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

private:
	http_handlers_t* handlers_;

	bool doService(int type, HttpRequest& req, HttpResponse& res)
	{
		// 如果需要 http session 控制，请打开下面注释，且需要保证
		// 在 master_service.cpp 的函数 thread_on_read 中设置的
		// memcached 服务正常工作
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

		if (type < http_handler_get || type >= http_handler_max) {
			logger_error("invalid type=%d", type);
			return false;
		}

		res.setKeepAlive(req.isKeepAlive());
		bool keep = req.isKeepAlive();

		const char* path = req.getPathInfo();
		if (path == NULL || *path == 0) {
			res.setStatus(400);
			acl::string buf("404 bad request\r\n");
			res.setContentLength(buf.size());
			return res.write(buf.c_str(), buf.size()) && keep;
		}

		size_t len = strlen(path);
		acl::string buf(path);
		if (path[len - 1] != '/') {
			buf += '/';
		}
		buf.lower();

		std::map<acl::string, http_handler_t>::iterator it
			= handlers_[type].find(buf);

		if (it != handlers_[type].end()) {
			return it->second(req, res) && keep;
		}

		res.setStatus(404);
		buf  = "404 ";
		buf += path;
		buf += " not found\r\n";
		res.setContentLength(buf.size());
		return res.write(buf.c_str(), buf.size()) && keep;
	}
};
