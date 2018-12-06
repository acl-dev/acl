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
	// @override
	bool doError(acl::HttpServletRequest&, acl::HttpServletResponse&);

	// @override
	bool doUnknown(acl::HttpServletRequest&, acl::HttpServletResponse&);

	// @override
	bool doGet(acl::HttpServletRequest&, acl::HttpServletResponse&);

	// @override
	bool doPost(acl::HttpServletRequest&, acl::HttpServletResponse&);

	// @override
	bool doWebSocket(acl::HttpServletRequest&, acl::HttpServletResponse&);

private:
	acl::session* session_;

	bool doPing(acl::websocket&, acl::websocket&);
	bool doPong(acl::websocket&, acl::websocket&);
	bool doClose(acl::websocket&, acl::websocket&);
	bool doMsg(acl::websocket&, acl::websocket&);

	bool getFilename(acl::websocket&, acl::string&);
	long long getFilesize(acl::websocket& in);
	bool saveFile(acl::websocket&, const acl::string&, long long);
	bool saveFile(acl::websocket&, acl::ofstream&, long long);
	int saveFile(acl::websocket&, acl::ofstream&);
};
