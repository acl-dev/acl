#pragma once

struct response_t;

class user_status {
public:
	user_status(const char* addr, acl::sslbase_conf& ssl_conf,
		const char* stok);
	~user_status(void);

	bool start(void);

private:
	acl::string addr_;
	acl::sslbase_conf& ssl_conf_;
	acl::string stok_;

	bool get_status(const char* stok);
	void build_request(acl::string& buf);
	bool parse_response(const acl::string& data, response_t& res);
	void show_status(const response_t& res);
};
