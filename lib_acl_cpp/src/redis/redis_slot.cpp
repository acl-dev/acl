#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/redis/redis_slot.hpp"
#endif

namespace acl
{

redis_slot::redis_slot(size_t slot_min, size_t slot_max,
	const char* ip, int port)
{
	slot_min_ = slot_min;
	slot_max_ = slot_max;
	ACL_SAFE_STRNCPY(ip_, ip, sizeof(ip_));
	port_ = port;
}

redis_slot::redis_slot(const redis_slot& node)
{
	slot_min_ = node.get_slot_min();
	slot_max_ = node.get_slot_max();
	ACL_SAFE_STRNCPY(ip_, node.get_ip(), sizeof(ip_));
	port_ = node.get_port();

	const std::vector<redis_slot*>& slaves = node.get_slaves();
	std::vector<redis_slot*>::const_iterator cit;
	for (cit = slaves.begin(); cit != slaves.end(); ++cit)
		slaves_.push_back(*cit);
}

redis_slot::~redis_slot()
{
	std::vector<redis_slot*>::iterator it;
	for (it = slaves_.begin(); it != slaves_.end(); ++it)
		delete *it;
}

redis_slot& redis_slot::add_slave(redis_slot* node)
{
	slaves_.push_back(node);
	return *this;
}

} // namespace acl
