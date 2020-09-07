#include "stdafx.h"
#include "http_service.h"

static bool http_not_found(const char* path, HttpRequest& req, HttpResponse& res)
{
	bool keep = req.isKeepAlive();

	res.setStatus(404);
	acl::string buf  = "404 ";
	buf += path;
	buf += " not found\r\n";
	res.setContentLength(buf.size());
	return res.write(buf.c_str(), buf.size()) && keep;
}

http_service::http_service(void) : handler_default_(http_not_found) {}
http_service::~http_service(void) {}

http_service& http_service::Default(http_default_handler_t fn)
{
	handler_default_ = fn;
	return *this;
}

http_service& http_service::Get(const char* path, http_handler_t fn)
{
	Service(http_handler_get, path, fn);
	return *this;
}

http_service& http_service::Post(const char* path, http_handler_t fn)
{
	Service(http_handler_post, path, fn);
	return *this;
}

http_service& http_service::Head(const char* path, http_handler_t fn)
{
	Service(http_handler_head, path, fn);
	return *this;
}

http_service& http_service::Put(const char* path, http_handler_t fn)
{
	Service(http_handler_put, path, fn);
	return *this;
}

http_service& http_service::Patch(const char* path, http_handler_t fn)
{
	Service(http_handler_patch, path, fn);
	return *this;
}

http_service& http_service::Connect(const char* path, http_handler_t fn)
{
	Service(http_handler_connect, path, fn);
	return *this;
}

http_service& http_service::Purge(const char* path, http_handler_t fn)
{
	Service(http_handler_purge, path, fn);
	return *this;
}

http_service& http_service::Delete(const char* path, http_handler_t fn)
{
	Service(http_handler_delete, path, fn);
	return *this;
}

http_service& http_service::Options(const char* path, http_handler_t fn)
{
	Service(http_handler_options, path, fn);
	return *this;
}

http_service& http_service::Propfind(const char* path, http_handler_t fn)
{
	Service(http_handler_profind, path, fn);
	return *this;
}

http_service& http_service::Websocket(const char* path, http_handler_t fn)
{
	Service(http_handler_websocket, path, fn);
	return *this;
}

http_service& http_service::Unknown(const char* path, http_handler_t fn)
{
	Service(http_handler_unknown, path, fn);
	return *this;
}

http_service& http_service::Error(const char* path, http_handler_t fn)
{
	Service(http_handler_error, path, fn);
	return *this;
}

void http_service::Service(int type, const char* path, http_handler_t fn)
{
	if (type >= http_handler_get && type < http_handler_max && path && *path) {
		// The path should lookup like as "/xxx/" with
		// lower charactors.

		acl::string buf(path);
		if (buf[buf.size() - 1] != '/') {
			buf += '/';
		}
		buf.lower();
		handlers_[type][buf] = fn;
	}
}

bool http_service::doService(int type, HttpRequest& req, HttpResponse& res)
{
	if (type < http_handler_get || type >= http_handler_max) {
		logger_error("invalid type=%d", type);
		return false;
	}

	res.setKeepAlive(req.isKeepAlive());
	bool keep = req.isKeepAlive();

	const char* path = req.getPathInfo();
	if (path == NULL || *path == 0) {
		res.setStatus(400);
		acl::string buf("400 bad request\r\n");
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

	return handler_default_(buf, req, res) && keep;
}
