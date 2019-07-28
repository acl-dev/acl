#pragma once

class http_servlet : public acl::HttpServlet
{
public:
	http_servlet(acl::redis_client_cluster& cluster, size_t max_conns);
	~http_servlet();

	acl::session& get_session() const
	{
		return *session_;
	}

protected:
	virtual bool doUnknown(acl::HttpServletRequest&,
		acl::HttpServletResponse& res);
	virtual bool doGet(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	virtual bool doPost(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);

private:
	acl::session* session_;
};
