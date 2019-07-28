#pragma once

class redis_status
{
public:
	redis_status(const char* addr, int conn_timeout, int rw_timeout,
		const char* passwd);
	~redis_status(void);

	void show_nodes(void);
	static void show_nodes(acl::redis& redis);

	static void show_nodes_tree(
		const std::map<acl::string, acl::redis_node*>&);
	static void show_nodes_tree(const std::vector<acl::redis_node*>& nodes);
	static void show_slave_tree(const std::vector<acl::redis_node*>& slaves);

	static bool show_nodes(const std::map<acl::string,
		acl::redis_node*>* masters);

	static void show_master_slots(const acl::redis_node* master);
	static void show_slave_nodes(const std::vector<acl::redis_node*>& slaves);

	void show_slots(void);
	static bool show_slots(acl::redis& redis);
	static void show_slaves_slots(const acl::redis_slot* slot);

private:
	acl::string addr_;
	int conn_timeout_;
	int rw_timeout_;
	acl::string passwd_;
};
