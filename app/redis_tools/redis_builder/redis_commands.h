#pragma once

class redis_commands
{
public:
	redis_commands(const char* addr, const char* passwd,
		int conn_timeout, int rw_timeout, bool prefer_master);
	~redis_commands(void);

	void run(void);

private:
	acl::string addr_;
	acl::string passwd_;
	int conn_timeout_;
	int rw_timeout_;
	bool prefer_master_;
	acl::redis_client_cluster* conns_;

	void set_addr(const char* in, acl::string& out);
	void getline(acl::string& buf, const char* prompt = NULL);
	void create_cluster();
	void help(void);
	void set_server(const std::vector<acl::string>& tokens);
	void show_nodes(void);
	void show_date(void);

	void get_keys(const std::vector<acl::string>& tokens);
	int get_keys(const char* addr, const char* pattern, int max);

	void getn(const std::vector<acl::string>& tokens);
	void get(const std::vector<acl::string>& tokens);
	void get(const char* key, int max);
	void hash_get(const std::vector<acl::string>& tokens);
	void hash_get(const char* key, size_t max);
	void string_get(const std::vector<acl::string>& tokens);
	void string_get(const char* key);
	void list_get(const std::vector<acl::string>& tokens);
	void list_get(const char* key, size_t max);
	void set_get(const std::vector<acl::string>& tokens);
	void set_get(const char* key, size_t max);
	void zset_get(const std::vector<acl::string>& tokens);
	void zset_get(const char* key, size_t max);

	void pattern_remove(const std::vector<acl::string>& tokens);
	int pattern_remove(const char* addr, const char* pattern);
	int remove(const std::vector<acl::string>& keys);

	void check_type(const std::vector<acl::string>& tokens);
	void check_ttl(const std::vector<acl::string>& tokens);
	void get_dbsize(const std::vector<acl::string>& tokens);
	void request(const std::vector<acl::string>& tokens);
	bool show_result(const acl::redis_result& result);

	void show_request(const std::vector<acl::string>& tokens);

	void config(const std::vector<acl::string>& tokens);
	void config(const char* addr, const std::vector<acl::string>& tokens);
	void config_usage(void);
	void request_one(const char* addr,
		const std::vector<acl::string>& tokens);
};
