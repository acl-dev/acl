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
	bool doApp(acl::HttpServletRequest&, acl::HttpServletResponse&);
	bool doDoc(acl::HttpServletRequest&, acl::HttpServletResponse&,
		const char* path);

	bool doReply(acl::HttpServletRequest&,
		acl::HttpServletResponse&, int status, const char* fmt, ...);
	bool doReply(acl::HttpServletRequest&,
		acl::HttpServletResponse&, int status, acl::json&);

	bool xmlToJson(acl::xml& xml, acl::json& json);
	void getContentType(const acl::string& filepath, acl::string& out);

private:
	acl::connect_pool* conns_;
};
