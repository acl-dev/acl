#pragma once

class http_service;

class http_servlet : public acl::HttpServlet
{
public:
	http_servlet(http_service& service, acl::socket_stream* conn,
		acl::session* session);
	~http_servlet(void);

protected:
	// @override
	bool doGet(HttpRequest& req, HttpResponse& res);

	// @override
	bool doPost(HttpRequest& req, HttpResponse& res);

	// @override
	bool doHead(HttpRequest& req, HttpResponse& res);

	// @override
	bool doPut(HttpRequest& req, HttpResponse& res);

	// @override
	bool doPatch(HttpRequest& req, HttpResponse& res);

	// @override
	bool doConnect(HttpRequest& req, HttpResponse& res);

	// @override
	bool doPurge(HttpRequest& req, HttpResponse& res);

	// @override
	bool doDelete(HttpRequest& req, HttpResponse& res);

	// @override
	bool doOptions(HttpRequest& req, HttpResponse& res);

	// @override
	bool doProfind(HttpRequest& req, HttpResponse& res);

	// @override
	bool doWebsocket(HttpRequest& req, HttpResponse& res);

	// @override
	bool doUnknown(HttpRequest& req, HttpResponse& res);

	// @override
	bool doError(HttpRequest& req, HttpResponse& res);

	// @override
	bool doOther(HttpRequest&, HttpResponse& res, const char* method);

private:
	http_service& service_;

	bool doService(int type, HttpRequest& req, HttpResponse& res);
};
