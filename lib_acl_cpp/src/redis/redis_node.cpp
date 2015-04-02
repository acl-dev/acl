#include "acl_stdafx.hpp"
#include "acl_cpp/redis/redis_node.hpp"

namespace acl
{

redis_node::redis_node(size_t slot_min, size_t slot_max,
	const char* ip, int port)
{
	slot_range_from_ = slot_min;
	slot_range_to_ = slot_max;
	ACL_SAFE_STRNCPY(ip_, ip, sizeof(ip_));
	port_ = port;
}

redis_node::~redis_node()
{
	std::vector<redis_node*>::iterator it;
	for (it = slaves_.begin(); it != slaves_.end(); ++it)
		delete *it;
}

redis_node& redis_node::add_slave(redis_node* node)
{
	slaves_.push_back(node);
	return *this;
}

} // namespace acl
