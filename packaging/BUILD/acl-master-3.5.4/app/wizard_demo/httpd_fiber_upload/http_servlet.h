#pragma once

class http_servlet : public acl::HttpServlet
{
public:
	http_servlet(acl::socket_stream*, acl::session*);
	~http_servlet(void);

protected:
	// @override
	bool doGet(request_t&, response_t&);

	// @override
	bool doPost(request_t&, response_t&);

	// @override
	bool doError(request_t&, response_t&);

	// @override
	bool doOther(request_t&, response_t&, const char* method);

private:
	typedef bool (http_servlet::*handler_t)(request_t&,response_t&);
	std::map<std::string, handler_t> handlers_;

	bool onPage(request_t& req, response_t& res);
	bool onUpload(request_t& req, response_t& res);
	bool upload(request_t& req, response_t& res, long long content_length,
		acl::ofstream& fp, acl::http_mime& mime);
	bool parse(request_t& req, response_t& res, acl::http_mime& mime);
	bool doReply(request_t& req, response_t& res, const char* info);
	long long get_fsize(const char* dir, const char* filename);

	void reset(void);

private:
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
