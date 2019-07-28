#pragma once
#include <vector>

class redis_migrate
{
public:
	redis_migrate(std::vector<acl::redis_node*>& masters,
		const char* passwd);
	~redis_migrate(void);

	int move_slots(std::vector<acl::redis_node*>& from,
		acl::redis_node& to, int count);
	int move_slots(acl::redis_node& from, acl::redis_node& to, int count);
	int move_slots(acl::redis& from, acl::redis& to, int count,
		const std::vector<std::pair<size_t, size_t> >& slots);
	int move_slots(acl::redis& from, acl::redis& to, int count,
		size_t min_slot, size_t max_slot);
	bool move_slot(size_t slot, acl::redis& from, acl::redis& to);
	bool move_key(const char* key, acl::redis& from, const char* to_addr);
	bool notify_cluster(size_t slot, const char* id);

private:
	std::vector<acl::redis_node*>& masters_;
	acl::string from_id_;
	acl::string to_id_;
	acl::string passwd_;

	bool check_nodes_id(acl::redis& from, acl::redis& to);
};
