#pragma once
#include "acl_cpp/http/HttpServlet.hpp"

class acl::HttpServletRequest;
class acl::HttpServletResponse;

class http_servet : public acl::HttpServlet
{
public:
	http_servlet();
	~http_servlet();

protected:
	virtual void doUnknown(acl::HttpServletRequest&,
		acl::HttpServletResponse& res);
	virtual bool doGet(acl::HttpServletRequest& req,
		HttpServletResponse& res);
	virtual bool doPost(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
};
