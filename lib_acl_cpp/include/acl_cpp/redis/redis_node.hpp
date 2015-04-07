#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include <utility>
#include "acl_cpp/stdlib/string.hpp"

namespace acl
{

class ACL_CPP_API redis_node
{
public:
	redis_node(const char* id, const char* addr);
	redis_node(const redis_node& node);
	~redis_node();

	void set_master(const redis_node* master);
	void set_master_id(const char* id);
	bool add_slave(redis_node* slave);
	const redis_node* remove_slave(const char* id);
	void clear_slaves(bool free_all = false);

	void add_slot_range(size_t min, size_t max);
	const std::vector<std::pair<size_t, size_t> >& get_slots() const
	{
		return slots_;
	}

	const redis_node* get_master() const
	{
		return master_;
	}

	const char* get_master_id() const
	{
		return master_id_.c_str();
	}

	const std::vector<redis_node*>* get_slaves() const
	{
		return (master_ && master_ == this) ? &slaves_ : NULL;
	}

	bool is_master() const
	{
		return master_ == this;
	}

	const char* get_id() const
	{
		return id_.c_str();
	}

	const char* get_addr() const
	{
		return addr_.c_str();
	}

private:
	string id_;
	string addr_;
	const redis_node* master_;
	string master_id_;
	std::vector<redis_node*> slaves_;
	std::vector<std::pair<size_t, size_t> > slots_;
};

} // namespace acl
