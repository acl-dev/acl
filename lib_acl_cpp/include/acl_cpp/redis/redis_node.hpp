#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>

namespace acl
{

class redis_node
{
public:
	redis_node(size_t slot_min, size_t slot_max, const char* ip, int port);
	~redis_node();

	redis_node& add_slave(redis_node* node);

	const std::vector<redis_node*>& get_slaves() const
	{
		return slaves_;
	}

	const char* get_ip() const
	{
		return ip_;
	}

	int get_port() const
	{
		return port_;
	}

	size_t get_slot_range_from() const
	{
		return slot_range_from_;
	}

	size_t get_slot_range_to() const
	{
		return slot_range_to_;
	}

private:
	size_t slot_range_from_;
	size_t slot_range_to_;
	char ip_[128];
	int port_;

	std::vector<redis_node*> slaves_;
};

} // namespace acl
