#pragma once

class http_servlet : public acl::HttpServlet
{
public:
	http_servlet(acl::socket_stream* stream, acl::session* session);
	~http_servlet(void);

	bool run(void);

protected:
	// @override
	bool doError(request_t&, response_t&);

	// @override
	bool doOther(request_t&, response_t&,
		const char* method);

	// @override
	bool doGet(request_t&, response_t&);

	// @override
	bool doPost(request_t&, response_t&);

private:
	typedef bool (http_servlet::*handler_t)(request_t&,response_t&);

	std::map<acl::string, handler_t> handlers_;

	bool doUpload(request_t&, response_t&);
	bool doReply(request_t&, response_t&, const char* info);
	void reset(void);

	bool onPage(request_t&, response_t&);
	bool onUpload(request_t&, response_t&);

private:
	bool upload(request_t&, response_t&);
	bool parse(request_t&, response_t&);
	long long get_fsize(const char* dir, const char* filename);

private:
	bool uploading_;
	request_t*  req_;
	response_t* res_;
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
