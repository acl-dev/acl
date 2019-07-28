#pragma once

class redis_cmdump
{
public:
	redis_cmdump(const char* addr, int conn_timeout, int rw_timeout,
		const char* passwd, bool prefer_master);
	~redis_cmdump(void);

	void saveto(const char* filepath, bool dump_all);

private:
	acl::string addr_;
	int conn_timeout_;
	int rw_timeout_;
	acl::string passwd_;
	bool prefer_master_;

	void get_nodes(std::vector<acl::string>& addrs);
};
