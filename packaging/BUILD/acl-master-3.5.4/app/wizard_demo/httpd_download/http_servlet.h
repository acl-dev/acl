#pragma once

class http_servlet : public acl::HttpServlet
{
public:
	http_servlet(const char* filepath, acl::socket_stream* conn,
		acl::session* session);
	~http_servlet();

protected:
	virtual bool doError(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	virtual bool doUnknown(acl::HttpServletRequest&,
		acl::HttpServletResponse& res);
	virtual bool doGet(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	virtual bool doPost(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);

private:
	acl::string filepath_;
	bool reply(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res, int status,
		const char* fmt, ...);
	bool transfer_file(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	bool transfer_file(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res,
		long long range_from, long long range_to);
};
