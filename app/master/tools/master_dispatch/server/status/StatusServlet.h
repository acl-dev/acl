#pragma once

class StatusServlet : public acl::HttpServlet
{
public:
	StatusServlet();
	~StatusServlet();

	bool keep_alive() const
	{
		return keep_alive_;
	}

protected:
	bool doGet(acl::HttpServletRequest& req, acl::HttpServletResponse& res);
	bool doPost(acl::HttpServletRequest& req, acl::HttpServletResponse& res);

private:
	bool keep_alive_;
};
