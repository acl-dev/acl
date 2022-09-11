#pragma once

struct response_t;

class http_status {
public:
	http_status(const char* addr, acl::sslbase_conf& ssl_conf,
		const char* user, const char* pass);
	~http_status(void);

	bool start(void);

private:
	acl::string addr_;
	acl::sslbase_conf& ssl_conf_;
	acl::string user_;
	acl::string pass_;

	bool login(acl::string& out);
	bool get_status(const char* stok);
	void build_request(acl::string& buf);
	bool parse_response(const acl::string& data, response_t& res);
	void show_status(const response_t& res);
};
