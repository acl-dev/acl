#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/redis/redis_node.hpp"

namespace acl
{

redis_node::redis_node(const char* id, const char* addr)
	: id_(id)
	, addr_(addr)
	, master_(NULL)
{

}

redis_node::redis_node(const redis_node& node)
{
	id_ = node.get_id();
	addr_ = node.get_addr();
	master_ = node.get_master();
	const std::vector<redis_node*>* slaves = node.get_slaves();
	if (slaves != NULL)
	{
		std::vector<redis_node*>::const_iterator cit;
		for (cit = slaves_.begin(); cit != slaves_.end(); ++cit)
			slaves_.push_back(*cit);
	}

	const std::vector<std::pair<size_t, size_t> >& slots = node.get_slots();
	std::vector<std::pair<size_t, size_t> >::const_iterator cit2;
	for (cit2 = slots.begin(); cit2 != slots.end(); ++cit2)
		slots_.push_back(*cit2);
}

redis_node::~redis_node()
{

}

void redis_node::set_master(const redis_node* master)
{
	master_ = master;
}

void redis_node::set_master_id(const char* id)
{
	if (id && *id)
		master_id_ = id;
}

bool redis_node::add_slave(redis_node* slave)
{
	if (slave == NULL)
		return false;
	std::vector<redis_node*>::const_iterator cit;
	for (cit = slaves_.begin(); cit != slaves_.end(); ++cit)
	{
		if (*cit == slave)
		{
			logger_warn("slave exists, id: %s, addr: %s",
				(*cit)->get_id(), (*cit)->get_addr());
			return false;
		}
	}

	slaves_.push_back(slave);
	return true;
}

const redis_node* redis_node::remove_slave(const char* id)
{
	std::vector<redis_node*>::iterator it;
	for (it = slaves_.begin(); it != slaves_.end(); ++it)
	{
		if (strcmp((*it)->get_id(), id) == 0)
		{
			slaves_.erase(it);
			return *it;
		}
	}

	return NULL;
}

void redis_node::clear_slaves(bool free_all /* = false */)
{
	if (free_all)
	{
		std::vector<redis_node*>::iterator it;
		for (it = slaves_.begin(); it != slaves_.end(); ++it)
			delete *it;
	}

	slaves_.clear();
}

void redis_node::add_slot_range(size_t min, size_t max)
{
	std::pair<size_t, size_t> range = std::make_pair(min, max);
	slots_.push_back(range);
}

} // namespace acl
