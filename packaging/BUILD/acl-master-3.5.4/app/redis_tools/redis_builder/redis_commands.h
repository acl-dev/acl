#pragma once

struct REDIS_CMD
{
	acl::string cmd;
	acl::string broadcast;
	acl::string perm;
};

class redis_commands
{
public:
	redis_commands(const char* addr, const char* passwd, int conn_timeout,
		int rw_timeout, bool prefer_master, const char* cmds_file);
	~redis_commands(void);

	void run(void);

private:
	acl::string addr_;
	acl::string passwd_;
	int conn_timeout_;
	int rw_timeout_;
	bool prefer_master_;
	acl::redis_client_cluster* conns_;
	std::map<acl::string, REDIS_CMD> redis_cmds_;
	acl::string all_cmds_perm_;

	void init(const char* cmds_file);
	void set_commands(void);
	void load_commands(acl::istream& in);
	void add_cmdline(acl::string& line, size_t i);
	void show_commands(void);
	bool check(const char* command);

	bool parse(acl::string& line, std::vector<acl::string>& out);
	void set_addr(const char* in, acl::string& out);
	void getline(acl::string& buf, const char* prompt = NULL);
	void create_cluster(void);
	void help(void);
	void set_server(const std::vector<acl::string>& tokens);
	void show_nodes(void);
	void show_date(void);

	void get_keys(const std::vector<acl::string>& tokens);
	int get_keys(const char* addr, const char* pattern, int max);

	void scan_keys(const std::vector<acl::string>& tokens);
	int scan(const char* addr, const char* pattern, size_t display_count);

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
	long long pattern_remove(const char* addr, const char* pattern,
		int cocurrent);
	void remove(const std::vector<acl::string>& keys,
		acl::atomic_long& deleted, acl::atomic_long& error,
		acl::atomic_long& notfound);
	void parallel_remove(int cocurrent,
		const std::vector<acl::string>& keys,
		acl::atomic_long& deleted, acl::atomic_long& error,
		acl::atomic_long& notfound);

	void check_type(const std::vector<acl::string>& tokens);
	void check_ttl(const std::vector<acl::string>& tokens);
	void get_dbsize(const std::vector<acl::string>& tokens);
	void request(const std::vector<acl::string>& tokens);
	void request_one(const std::vector<acl::string>& tokens);
	void request_all(const std::vector<acl::string>& tokens);
	void request_masters(const std::vector<acl::string>& tokens);
	void request_slaves(const std::vector<acl::string>& tokens);
	bool show_result(const acl::redis_result& result, const char* addr);

	void show_request(const std::vector<acl::string>& tokens);

	void config(const std::vector<acl::string>& tokens);
	void config(const char* addr, const std::vector<acl::string>& tokens);
	void config_usage(void);
	void request_one(const char* addr,
		const std::vector<acl::string>& tokens);
};
