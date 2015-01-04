#pragma once

class message;
class message_manager;

class client_servlet : public acl::HttpServlet
{
public:
	client_servlet(const char* domain, int port);
	~client_servlet();

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
	acl::string domain_;
	int port_;
	std::vector<acl::string> servers_;

	bool doLogin(const char* user, const char* pass);
	bool doAction(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	bool doRequest(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	bool doResponse(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);

	bool get_servers();
	void lookup_dns();
	bool show_page(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res, const char* fmt, ...);
	bool show_login(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	bool wait_result(acl::HttpServletResponse& res,
		message_manager& manager, int nthreads);
	bool reply(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res, const char* fmt, ...);
	bool reply_status(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res, int status, const char* fmt, ...);
	int reply(acl::HttpServletResponse& res, message& msg);
};
