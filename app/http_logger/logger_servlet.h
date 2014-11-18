#pragma once

class logger_servlet : public acl::HttpServlet
{
public:
	logger_servlet();
	~logger_servlet();

protected:
	virtual bool doUnknown(acl::HttpServletRequest&,
		acl::HttpServletResponse& res);
	virtual bool doGet(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	virtual bool doPost(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);

private:
};
