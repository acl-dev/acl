#pragma once

class http_servlet : public acl::HttpServlet
{
public:
	http_servlet(http_handlers_t*, acl::socket_stream*, acl::session*);
	~http_servlet(void);

protected:
	// @override
	bool doGet(HttpRequest&, HttpResponse&);

	// @override
	bool doPost(HttpRequest&, HttpResponse&);

	// override
	bool doHead(HttpRequest& req, HttpResponse& res);

	// override
	bool doPut(HttpRequest& req, HttpResponse& res);

	// override
	bool doPatch(HttpRequest& req, HttpResponse& res);

	// override
	bool doConnect(HttpRequest& req, HttpResponse& res);

	// override
	bool doPurge(HttpRequest& req, HttpResponse& res);

	// override
	bool doDelete(HttpRequest& req, HttpResponse& res);

	// override
	bool doOptions(HttpRequest& req, HttpResponse& res);

	// override
	bool doProfind(HttpRequest& req, HttpResponse& res);

	// override
	bool doWebsocket(HttpRequest& req, HttpResponse& res);

	// override
	bool doUnknown(HttpRequest& req, HttpResponse& res);

	// @override
	bool doError(HttpRequest&, HttpResponse&);

	// @override
	bool doOther(HttpRequest&, HttpResponse&, const char* method);

private:
	http_handlers_t* handlers_;

	bool doService(int type, HttpRequest& req, HttpResponse& res);
};
