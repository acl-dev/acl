#pragma once

class http_servlet : public acl::HttpServlet
{
public:
	http_servlet(acl::socket_stream* stream, acl::session* session);
	~http_servlet();

	bool run(void);

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
	bool doBody(acl::HttpServletRequest&, acl::HttpServletResponse&);
	bool doUpload(acl::HttpServletRequest&, acl::HttpServletResponse&);
	bool doParse(acl::HttpServletRequest&, acl::HttpServletResponse&);
	bool doReply(acl::HttpServletRequest&, acl::HttpServletResponse&,
		const char* info);
	void reset(void);

	long long get_fsize(const char* dir, const char* filename);

private:
	bool read_body_;
	acl::HttpServletRequest* req_;
	acl::HttpServletResponse* res_;
	acl::ofstream fp_;
	long long content_length_;
	long long read_length_;
	acl::http_mime* mime_;

	acl::string param1_;
	acl::string param2_;
	acl::string param3_;
	acl::string file1_;
	acl::string file2_;
	acl::string file3_;
	long long   fsize1_;
	long long   fsize2_;
	long long   fsize3_;
};
