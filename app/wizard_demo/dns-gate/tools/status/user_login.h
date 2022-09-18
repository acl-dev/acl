#pragma once

class user_login {
public:
	user_login(const char* addr, acl::sslbase_conf& ssl_conf,
		const char* user, const char* pass);
	~user_login(void);

	bool start(acl::string& stok);

private:
	acl::string addr_;
	acl::sslbase_conf& ssl_conf_;
	acl::string user_;
	acl::string pass_;
};
