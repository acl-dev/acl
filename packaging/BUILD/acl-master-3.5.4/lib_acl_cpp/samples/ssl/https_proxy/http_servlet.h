#pragma once

class http_servlet : public acl::HttpServlet
{
public:
	http_servlet(acl::ostream& out, acl::sslbase_conf* conf);
	~http_servlet();

protected:
	bool doGet(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	bool doPost(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	bool doUnknown(acl::HttpServletRequest&,
		acl::HttpServletResponse& res);
	bool doError(acl::HttpServletRequest&,
		acl::HttpServletResponse& res);
	bool doConnect(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	bool doPut(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	bool doDelete(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	bool doHead(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	bool doOptions(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	bool doPropfind(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	bool doOther(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res, const char* method);

private:
	bool handled_;
	acl::string url_;
	acl::ostream& out_;
	acl::sslbase_conf* client_ssl_conf_;

	void logger_request(acl::HttpServletRequest& req);
};
