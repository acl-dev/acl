#pragma once

class http_servlet : public acl::HttpServlet
{
public:
	http_servlet(acl::socket_stream*, acl::session*);
	~http_servlet();

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

	bool on_default(request_t&, response_t&);
	bool on_hello(request_t&, response_t&);
};
