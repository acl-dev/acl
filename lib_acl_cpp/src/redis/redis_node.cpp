#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/redis/redis_node.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

redis_node::redis_node()
	: myself_(false)
	, handshaking_(false)
	, connected_(false)
	, master_(NULL)
{
}

redis_node::~redis_node()
{

}

redis_node& redis_node::set_id(const char* id)
{
	id_ = id;
	return *this;
}

redis_node& redis_node::set_addr(const char* addr)
{
	addr_info_ = addr;
	int pos = addr_info_.find('@');
	if (pos <= 0)
		addr_ = addr_info_;
	else
		addr_.copy(addr_info_, pos);
	return *this;
}

redis_node& redis_node::set_type(const char* type)
{
	type_ = type;
	return *this;
}

redis_node& redis_node::set_myself(bool yesno)
{
	myself_ = yesno;
	return *this;
}

redis_node& redis_node::set_handshaking(bool yesno)
{
	handshaking_ = yesno;
	return *this;
}

redis_node& redis_node::set_connected(bool yesno)
{
	connected_ = yesno;
	return *this;
}

redis_node& redis_node::set_master(const redis_node* master)
{
	master_ = master;
	return *this;
}

redis_node& redis_node::set_master_id(const char* id)
{
	if (id && *id)
		master_id_ = id;
	return *this;
}

bool redis_node::add_slave(redis_node* slave)
{
	if (slave == NULL)
		return false;
	std::vector<redis_node*>::const_iterator cit;
	for (cit = slaves_.begin(); cit != slaves_.end(); ++cit) {
		if (*cit == slave) {
			printf("slave exists: %s, id: %s, addr: %s\r\n",
				slave->get_id(), (*cit)->get_id(),
				(*cit)->get_addr());
			return false;
		}
		if ((*slave->get_id()) == 0)
			continue;
		if (strcmp(slave->get_id(), (*cit)->get_id()) == 0) {
			printf("slave exists: %s, id: %s, addr: %s\r\n",
				slave->get_id(), (*cit)->get_id(),
				(*cit)->get_addr());
			return false;
		}
	}

	slaves_.push_back(slave);
	return true;
}

redis_node* redis_node::remove_slave(const char* id)
{
	std::vector<redis_node*>::iterator it;
	for (it = slaves_.begin(); it != slaves_.end(); ++it) {
		if (strcmp((*it)->get_id(), id) == 0) {
			slaves_.erase(it);
			return *it;
		}
	}

	return NULL;
}

void redis_node::clear_slaves(bool free_all /* = false */)
{
	if (free_all) {
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

const std::vector<std::pair<size_t, size_t> >& redis_node::get_slots() const
{
	if (is_master())
		return slots_;
	else if (master_ != NULL)
		return master_->get_slots();
	else {
		//logger_warn("not master and not slave!");
		return slots_;
	}
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
