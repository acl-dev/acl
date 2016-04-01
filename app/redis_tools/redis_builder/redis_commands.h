#pragma once

class redis_commands
{
public:
	redis_commands(const char* addr, const char* passwd,
		int conn_timeout, int rw_timeout);
	~redis_commands(void);

	void run(void);

private:
	acl::string addr_;
	acl::string passwd_;
	int conn_timeout_;
	int rw_timeout_;
	acl::redis_client_cluster conns_;
	acl::redis_client conn_;

	const std::map<acl::string, acl::redis_node*>* get_masters(acl::redis&);
	void help(void);
	void show_date(void);

	void get_keys(const std::vector<acl::string>& tokens);
	int get_keys(const char* addr, const char* pattern);

	void hgetall(const std::vector<acl::string>& tokens);

	void pattern_remove(const std::vector<acl::string>& tokens);
	int remove(const std::vector<acl::string>& keys);

	void check_type(const std::vector<acl::string>& tokens);
	void check_ttl(const std::vector<acl::string>& tokens);
};
