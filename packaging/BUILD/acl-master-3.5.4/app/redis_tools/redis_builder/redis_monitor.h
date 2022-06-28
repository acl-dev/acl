#pragma once

class redis_monitor
{
public:
	redis_monitor(const char* addr, int conn_timeout, int rw_timeout,
		const char* passwd, bool prefer_master);
	~redis_monitor(void);

	void status(void);

private:
	acl::string addr_;
	int conn_timeout_;
	int rw_timeout_;
	acl::string passwd_;
	bool prefer_master_;

	void show_status(std::vector<acl::redis_client*>& conns);
	int check(const std::map<acl::string, acl::string>& info,
		const char* name, std::vector<int>& res);
	long long check(const std::map<acl::string, acl::string>& info,
		const char* name, std::vector<long long>& res);
	double check(const std::map<acl::string, acl::string>& info,
		const char* name, std::vector<double>& res);
	int check_keys(const char* name, const char* value);
	int check_keys(const std::map<acl::string, acl::string>& info,
		std::vector<int>& keys);
};
