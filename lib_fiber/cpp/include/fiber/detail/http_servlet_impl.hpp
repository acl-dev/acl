#pragma once

#include <map>
#include <string>
#include <sstream>
#include <functional>

namespace acl {

class HttpServletRequest;
class HttpServletResponse;

typedef HttpServletRequest  HttpRequest;
typedef HttpServletResponse HttpResponse;

typedef std::function<bool(HttpRequest&, HttpResponse&)> http_handler_t;
typedef std::map<acl::string, http_handler_t> http_handlers_t;

enum {
	http_handler_get = 0,
	http_handler_post,
	http_handler_head,
	http_handler_put,
	http_handler_patch,
	http_handler_connect,
	http_handler_purge,
	http_handler_delete,
	http_handler_options,
	http_handler_profind,
	http_handler_websocket,
	http_handler_error,
	http_handler_unknown,
	http_handler_max,
};

class http_servlet_impl : public HttpServlet {
public:
	http_servlet_impl(http_handlers_t* handlers,
		socket_stream* stream, session* session)
	: HttpServlet(stream, session), handlers_(handlers) {}

	virtual ~http_servlet_impl(void) {}

protected:
	// override
	bool doGet(HttpRequest& req, HttpResponse& res) {
		return doService(http_handler_get, req, res);
	}

	// override
	bool doPost(HttpRequest& req, HttpResponse& res) {
		return doService(http_handler_post, req, res);
	}

	// override
	bool doHead(HttpRequest& req, HttpResponse& res) {
		return doService(http_handler_head, req, res);
	}

	// override
	bool doPut(HttpRequest& req, HttpResponse& res) {
		return doService(http_handler_put, req, res);
	}

	// override
	bool doPatch(HttpRequest& req, HttpResponse& res) {
		return doService(http_handler_patch, req, res);
	}

	// override
	bool doConnect(HttpRequest& req, HttpResponse& res) {
		return doService(http_handler_connect, req, res);
	}

	// override
	bool doPurge(HttpRequest& req, HttpResponse& res) {
		return doService(http_handler_purge, req, res);
	}

	// override
	bool doDelete(HttpRequest& req, HttpResponse& res) {
		return doService(http_handler_delete, req, res);
	}

	// override
	bool doOptions(HttpRequest& req, HttpResponse& res) {
		return doService(http_handler_options, req, res);
	}

	// override
	bool doProfind(HttpRequest& req, HttpResponse& res) {
		return doService(http_handler_profind, req, res);
	}

	// override
	bool doWebsocket(HttpRequest& req, HttpResponse& res) {
		return doService(http_handler_websocket, req, res);
	}

	// override
	bool doUnknown(HttpRequest& req, HttpResponse& res) {
		return doService(http_handler_unknown, req, res);
	}

	// override
	bool doError(HttpRequest& req, HttpResponse& res) {
		return doService(http_handler_error, req, res);
	}

private:
	bool doService(int type, HttpRequest& req, HttpResponse& res) {
		if (type < http_handler_get || type >= http_handler_max) {
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
		buf = "404 ";
		buf += path;
		buf += " not found\r\n";
		res.setContentLength(buf.size());
		return res.write(buf.c_str(), buf.size()) && keep;
	}

private:
	http_handlers_t* handlers_;
};

} // namespace acl
