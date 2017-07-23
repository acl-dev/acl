#pragma once

class message;
class message_manager;

class server_servlet : public acl::HttpServlet
{
public:
	server_servlet();
	~server_servlet();

protected:
	virtual bool doUnknown(acl::HttpServletRequest&,
		acl::HttpServletResponse& res);
	virtual bool doGet(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	virtual bool doPost(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	virtual bool doError(acl::HttpServletRequest& req,
		acl::HttpServletResponse&);

private:
	bool doXml(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	bool doJson(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	bool reply(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res, const char* fmt, ...);
	bool reply_status(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res,
		int status, const char* fmt, ...);
};
