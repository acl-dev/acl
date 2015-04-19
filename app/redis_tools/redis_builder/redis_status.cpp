#include "stdafx.h"
#include "redis_status.h"

redis_status::redis_status(const char* addr, int conn_timeout, int rw_timeout)
	: addr_(addr)
	, conn_timeout_(conn_timeout)
	, rw_timeout_(rw_timeout)
{
}


redis_status::~redis_status(void)
{
}

//////////////////////////////////////////////////////////////////////////

void redis_status::show_nodes()
{
	acl::redis_client client(addr_, conn_timeout_, rw_timeout_);
	acl::redis redis(&client);

	show_nodes(redis);
}

void redis_status::show_nodes(acl::redis& redis)
{
	const std::map<acl::string, acl::redis_node*>* masters;
	if ((masters = redis.cluster_nodes())== NULL)
		printf("can't get cluster nodes\r\n");
	else
		show_nodes(masters);
}

void redis_status::show_slave_nodes(
	const std::vector<acl::redis_node*>& slaves)
{
	std::vector<acl::redis_node*>::const_iterator cit;
	for (cit = slaves.begin(); cit != slaves.end(); ++cit)
	{
		printf("slave, id: %s, addr: %s, master_id: %s\r\n",
			(*cit)->get_id(), (*cit)->get_addr(),
			(*cit)->get_master_id());
	}
}

void redis_status::show_master_slots(const acl::redis_node* master)
{
	const std::vector<std::pair<size_t, size_t> >& slots =
		master->get_slots();

	std::vector<std::pair<size_t, size_t> >::const_iterator cit;
	for (cit = slots.begin(); cit != slots.end(); ++cit)
		printf("slots range: %d-%d\r\n",
		(int) (*cit).first, (int) (*cit).second);
}

bool redis_status::show_nodes(
	const std::map<acl::string, acl::redis_node*>* masters)
{
	const std::vector<acl::redis_node*>* slaves;
	std::map<acl::string, acl::redis_node*>::const_iterator cit;
	for (cit = masters->begin(); cit != masters->end(); ++cit)
	{
		if (cit != masters->begin())
			printf("---------------------------------------\r\n");
		
		printf("master, id: %s, addr: %s\r\n",
			cit->first.c_str(), cit->second->get_addr());
		show_master_slots(cit->second);
		slaves = cit->second->get_slaves();
		show_slave_nodes(*slaves);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

void redis_status::show_slots()
{
	acl::redis_client client(addr_, conn_timeout_, rw_timeout_);
	acl::redis redis(&client);

	show_slots(redis);
}

bool redis_status::show_slots(acl::redis& redis)
{
	const std::vector<acl::redis_slot*>* slots = redis.cluster_slots();
	if (slots == NULL)
		return false;

	std::vector<acl::redis_slot*>::const_iterator cit;

	for (cit = slots->begin(); cit != slots->end(); ++cit)
	{
		printf("=========================================\r\n");
		printf("master: ip: %s, port: %d, slots: %d - %d\r\n",
			(*cit)->get_ip(), (*cit)->get_port(),
			(int) (*cit)->get_slot_min(),
			(int) (*cit)->get_slot_max());
		show_slaves_slots(*cit);
	}

	return true;
}

void redis_status::show_slaves_slots(const acl::redis_slot* slot)
{
	const std::vector<acl::redis_slot*>& slaves = slot->get_slaves();
	std::vector<acl::redis_slot*>::const_iterator cit;
	for (cit = slaves.begin(); cit != slaves.end(); ++cit)
	{
		printf("slave: ip: %s, port: %d, slots: %d - %d\r\n",
			(*cit)->get_ip(), (*cit)->get_port(),
			(int) (*cit)->get_slot_min(),
			(int) (*cit)->get_slot_max());
	}
}
