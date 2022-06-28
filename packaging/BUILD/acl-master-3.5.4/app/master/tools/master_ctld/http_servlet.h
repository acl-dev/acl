#pragma once

class http_servlet : public acl::HttpServlet
{
public:
	http_servlet(acl::socket_stream*, acl::session*,
		const char* addr, const char* path);
	~http_servlet();

protected:
	// @override
	bool doError(acl::HttpServletRequest&, acl::HttpServletResponse&);

	// @override
	bool doOther(acl::HttpServletRequest&,
		acl::HttpServletResponse&, const char* method);

	// @override
	bool doGet(acl::HttpServletRequest&, acl::HttpServletResponse&);

	// @override
	bool doPost(acl::HttpServletRequest&, acl::HttpServletResponse&);

private:
	acl::string addr_;
	acl::string conf_;

	bool replyf(acl::HttpServletRequest&,
		acl::HttpServletResponse&, int status, const char* fmt, ...);
	bool reply(acl::HttpServletRequest&,
		acl::HttpServletResponse&, int status, const acl::string&);
	bool reply_json(acl::HttpServletRequest&, acl::HttpServletResponse&,
		int status, const acl::string&);
};
