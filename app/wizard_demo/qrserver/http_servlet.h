#pragma once

class http_servlet : public acl::HttpServlet
{
public:
	http_servlet(acl::socket_stream* stream, acl::session* session);
	~http_servlet();

protected:
	virtual bool doError(acl::HttpServletRequest&,
		acl::HttpServletResponse& res);
	virtual bool doOther(acl::HttpServletRequest&,
		acl::HttpServletResponse& res, const char* method);
	virtual bool doGet(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	virtual bool doPost(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);

private:
	bool replyf(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res,
		const char* fmt, ...);
};
