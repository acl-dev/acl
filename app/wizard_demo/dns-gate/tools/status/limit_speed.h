#pragma once

class limit_speed {
public:
	limit_speed(const char* addr, acl::sslbase_conf& ssl_conf,
		const char* stok, const char* mac, const char* ip);
	~limit_speed(void);

	limit_speed& set_hostname(const char* val);
	limit_speed& set_ssid(const char* val);

	bool start(int up, int down);

private:
	acl::string addr_;
	acl::sslbase_conf& ssl_conf_;
	acl::string stok_;
	acl::string mac_;
	acl::string ip_;

	acl::string hostname_;
	acl::string ssid_;

	void build_request(acl::string& buf, int up, int down);
};
