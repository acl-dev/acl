#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>

namespace acl
{

class ACL_CPP_API redis_slot
{
public:
	redis_slot(size_t slot_min, size_t slot_max,
		const char* ip, int port);
	redis_slot(const redis_slot& node);

	~redis_slot();

	redis_slot& add_slave(redis_slot* node);

	const std::vector<redis_slot*>& get_slaves() const
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

	size_t get_slot_min() const
	{
		return slot_min_;
	}

	size_t get_slot_max() const
	{
		return slot_max_;
	}

private:
	size_t slot_min_;
	size_t slot_max_;
	char ip_[128];
	int port_;

	std::vector<redis_slot*> slaves_;
};

} // namespace acl
