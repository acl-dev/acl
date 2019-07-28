#include "stdafx.h"
#include "redis_util.h"
#include "redis_status.h"

redis_status::redis_status(const char* addr, int conn_timeout, int rw_timeout,
	const char* passwd)
: addr_(addr)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
{
	if (passwd && *passwd) {
		passwd_ = passwd;
	}
}


redis_status::~redis_status(void)
{
}

//////////////////////////////////////////////////////////////////////////

void redis_status::show_nodes(void)
{
	acl::redis_client client(addr_, conn_timeout_, rw_timeout_);
	client.set_password(passwd_);
	acl::redis redis(&client);

	show_nodes(redis);
}

void redis_status::show_nodes(acl::redis& redis)
{
	const std::map<acl::string, acl::redis_node*>* masters;
	if ((masters = redis.cluster_nodes())== NULL) {
		printf("can't get cluster nodes\r\n");
		return;
	}

#ifdef ACL_UNIX
	show_nodes_tree(*masters);
#else
	show_nodes(masters);
#endif
}

//////////////////////////////////////////////////////////////////////////

void redis_status::show_nodes_tree(
	const std::map<acl::string, acl::redis_node*>& nodes)
{
	std::map<acl::string, std::vector<acl::redis_node*>* > sorted_nodes;
	redis_util::sort(nodes, sorted_nodes);

	std::map<acl::string, std::vector<acl::redis_node*>* >
		::const_iterator cit = sorted_nodes.begin();
	for (; cit != sorted_nodes.end(); ++cit) {
		printf("\033[1;31;40mmachine: %s\033[0m\r\n",
			cit->first.c_str());
		show_nodes_tree(*cit->second);
		printf("\r\n");
	}

	redis_util::clear_nodes_container(sorted_nodes);
}

void redis_status::show_nodes_tree(const std::vector<acl::redis_node*>& nodes)
{
	const std::vector<acl::redis_node*>* slaves;

	for (std::vector<acl::redis_node*>::const_iterator cit = nodes.begin();
		cit != nodes.end(); ++cit) {

		printf("\033[1;32;40m|--- master: %s\033[0m, "
			"\033[0;34;40mid: %s\033[0m, "
			"\033[1;33;40mslots\033[0m:",
			(*cit)->get_addr(), (*cit)->get_id());
		slaves = (*cit)->get_slaves();
		show_master_slots(*cit);
		printf("\r\n");
		show_slave_tree(*slaves);
	}
}

void redis_status::show_slave_tree(const std::vector<acl::redis_node*>& slaves)
{
	for (std::vector<acl::redis_node*>::const_iterator cit =
		slaves.begin(); cit != slaves.end(); ++cit) {

		printf("\t\033[1;32;40m|--- slave: %s\033[0m, "
			"\033[0;34;40mid: %s\033[0m\r\n",
			(*cit)->get_addr(), (*cit)->get_id());
	}
}

//////////////////////////////////////////////////////////////////////////

bool redis_status::show_nodes(
	const std::map<acl::string, acl::redis_node*>* masters)
{
	const std::vector<acl::redis_node*>* slaves;
	std::map<acl::string, acl::redis_node*>::const_iterator cit;
	for (cit = masters->begin(); cit != masters->end(); ++cit) {
		if (cit != masters->begin()) {
			printf("---------------------------------------\r\n");
		}
		
		printf("master, id: %s, addr: %s\r\n",
			cit->first.c_str(), cit->second->get_addr());

		printf("slots range: ");
		show_master_slots(cit->second);
		printf("\r\n");

		slaves = cit->second->get_slaves();
		show_slave_nodes(*slaves);
	}

	return true;
}

void redis_status::show_slave_nodes(
	const std::vector<acl::redis_node*>& slaves)
{
	std::vector<acl::redis_node*>::const_iterator cit;
	for (cit = slaves.begin(); cit != slaves.end(); ++cit) {
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
	for (cit = slots.begin(); cit != slots.end(); ++cit) {
#ifdef ACL_UNIX
		printf(" \033[1;33;40m[%d-%d]\033[0m",
			(int) (*cit).first, (int) (*cit).second);
#else
		printf(" [%d-%d]",
			(int) (*cit).first, (int) (*cit).second);
#endif
	}
}

//////////////////////////////////////////////////////////////////////////

void redis_status::show_slots(void)
{
	acl::redis_client client(addr_, conn_timeout_, rw_timeout_);
	client.set_password(passwd_);
	acl::redis redis(&client);

	show_slots(redis);
}

bool redis_status::show_slots(acl::redis& redis)
{
	const std::vector<acl::redis_slot*>* slots = redis.cluster_slots();
	if (slots == NULL) {
		return false;
	}

	std::vector<acl::redis_slot*>::const_iterator cit;

	for (cit = slots->begin(); cit != slots->end(); ++cit) {
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
	for (cit = slaves.begin(); cit != slaves.end(); ++cit) {
		printf("slave: ip: %s, port: %d, slots: %d - %d\r\n",
			(*cit)->get_ip(), (*cit)->get_port(),
			(int) (*cit)->get_slot_min(),
			(int) (*cit)->get_slot_max());
	}
}
