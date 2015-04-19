#include "stdafx.h"
#include "redis_status.h"
#include "redis_builder.h"

#define MAX_SLOTS 16384

redis_builder::redis_builder(int meet_wait /* = 100 */)
	: meet_wait_(meet_wait)
	, last_check_(0)
{
}

redis_builder::~redis_builder(void)
{
	std::vector<acl::redis_node*>::iterator it = masters_.begin();
	for (; it != masters_.end(); ++it)
		delete *it;
}

//////////////////////////////////////////////////////////////////////////

bool redis_builder::add_node(const char* addr,
	const char* new_node_addr, bool slave)
{
	acl::redis_client client(new_node_addr);
	acl::redis redis(&client);

	acl::string buf(addr);
	const std::vector<acl::string>& tokens = buf.split2(":");
	if (tokens.size() != 2)
	{
		printf("invalid addr: %s\r\n", addr);
		return false;
	}

	// CLUSTER MEET master node
	if (!redis.cluster_meet(tokens[0].c_str(), atoi(tokens[1].c_str())))
	{
		printf("cluster meet %s %s error: %s\r\n",
			tokens[0].c_str(), tokens[1].c_str(),
			redis.result_error());
		return false;
	}

	// wait for the master recognizing the slave
	while (true)
	{
		if (cluster_meeting(redis, addr) == true)
			break;
		acl_doze(meet_wait_);
	}

	if (!slave)
		return true;

	acl::string node_id;
	if (get_node_id(addr, node_id) == false)
	{
		printf("can't get master(%s)'s node_id\r\n", addr);
		return false;
	}

	if (redis.cluster_replicate(node_id.c_str()) == false)
	{
		printf("cluster replicate id: %s, error: %s, "
			"master_addr: %s, slave_addr: %s\r\n",
			node_id.c_str(), redis.result_error(),
			addr, new_node_addr);
		return false;
	}

	return true;
}

bool redis_builder::del_node(const char* addr, const char* node_id)
{
	acl::redis_client client(addr);
	acl::redis redis(&client);
	if (redis.cluster_forget(node_id) == false)
	{
		printf("del node: %s error: %s, addr: %s\r\n",
			node_id, redis.result_error(), addr);
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

bool redis_builder::build(const char* conf)
{
	if (load(conf) == false)
		return false;

	if (masters_.empty())
	{
		printf("no nodes available\r\n");
		return false;
	}

	printf("===================================================\r\n");

	return build_cluster();
}

bool redis_builder::load(const char* conf)
{
	acl::string buf;

	// load xml information from local file
	if (acl::ifstream::load(conf, &buf) == false)
	{
		printf("load configure error: %s, file: %s\r\n",
			acl::last_serror(), conf);
		return false;
	}
	/* <xml>
	 *   <node addr="ip:port">
	 *     <node addr="ip:port" />
	 *     <node addr="ip:port" />
	 *     ...
	 *   </node>
	 *   <node addr="ip:port">
	 *     <node addr="ip:port" />
	 *     <node addr="ip:port" />
	 *     ...
	 *   </node>
	 *   ...
	 * </xml>
	 */
	// parse the xml data
	acl::xml xml(buf.c_str());

	// get the master redis nodes
	const char* tags = "xml/node";
	const std::vector<acl::xml_node*>& nodes = xml.getElementsByTags(tags);

	if (nodes.empty())
	{
		printf("nodes null\r\n");
		return false;
	}

	// iterate all the master nodes including their's slaves
	std::vector<acl::xml_node*>::const_iterator cit;
	for (cit = nodes.begin(); cit != nodes.end(); ++cit)
	{
		acl::redis_node* master = create_master(**cit);
		if (master != NULL)
			masters_.push_back(master);
	}

	return true;
}

acl::redis_node* redis_builder::create_master(acl::xml_node& node)
{
	const char* addr = node.attr_value("addr");
	if (addr == NULL || *addr == 0)
	{
		printf("no addr in the master node\r\n");
		return NULL;
	}

	printf(">>> master: %s\r\n", addr);

	acl::redis_node* master = new acl::redis_node;
	master->set_addr(addr);

	// iterate all the slaves of the master, and add them to master
	acl::xml_node* child = node.first_child();
	while (child != NULL)
	{
		acl::redis_node* slave = create_slave(*child);
		if (slave != NULL)
			master->add_slave(slave);
		child = node.next_child();
	}

	//const std::vector<acl::redis_node*>* slaves = master->get_slaves();
	//printf(">>>slave's size: %d\r\n", (int) slaves->size());
	return master;
}

acl::redis_node* redis_builder::create_slave(acl::xml_node& node)
{
	const char* addr = node.attr_value("addr");
	if (addr == NULL || *addr == 0)
	{
		printf("no addr in the slave addr\r\n");
		return NULL;
	}

	printf("\tslave: %s\r\n", addr);
	acl::redis_node* slave = new acl::redis_node;
	slave->set_addr(addr);
	return slave;
}

bool redis_builder::build_cluster()
{
	if (masters_.empty())
	{
		printf("no master available!\r\n");
		return false;
	}

	size_t range = MAX_SLOTS / masters_.size();
	size_t begin = 0, end = MAX_SLOTS % masters_.size() + range -1;

	// build every master node, and connect all of its slaves.

	std::vector<acl::redis_node*>::iterator it;
	for (it = masters_.begin(); it != masters_.end(); ++it)
	{
		if (it != masters_.begin())
			printf("----------------------------------------\r\n");

		(*it)->add_slot_range(begin, end);
		if (build_master(**it) == false)
			return false;
		begin = end + 1;
		end = end + range;
	}

	it = masters_.begin();
	acl::redis_client client((*it)->get_addr());
	acl::redis master(&client);

	// let one master to connect all other master nodes

	printf("===================================================\r\n");
	printf("Meeting all masters and slaves ...\r\n");

	std::vector<acl::redis_node*> all_slaves;
	std::vector<acl::redis_node*>::const_iterator cit;

	for (++it; it != masters_.end(); ++it)
	{
		if (cluster_meet(master, **it) == false)
			return false;
		const std::vector<acl::redis_node*>* slaves = (*it)->get_slaves();
		for (cit = slaves->begin(); cit != slaves->end(); ++cit)
			all_slaves.push_back(*cit);
	}

	while (true)
	{
		int nwait = 0;
		for (cit = all_slaves.begin(); cit != all_slaves.end(); ++cit)
		{
			if ((*cit)->is_connected())
				continue;
			if (cluster_meeting(master, (*cit)->get_addr()) == false)
				nwait++;
			else
				(*cit)->set_connected(true);
		}
		if (nwait == 0)
			break;
		acl_doze(meet_wait_);
	}

	/////////////////////////////////////////////////////////////////////

	printf("===================================================\r\n");
	printf("All nodes of cluster:\r\n");

	const std::map<acl::string, acl::redis_node*>* nodes;
	if ((nodes = master.cluster_nodes())== NULL)
	{
		printf("can't get cluster nodes, addr: %s\r\n",
			client.get_stream()->get_peer(true));
		return false;
	}

	redis_status::show_nodes(nodes);
	return true;
}

bool redis_builder::cluster_meet(acl::redis& redis,
	const acl::redis_node& node)
{
	acl::string buf(node.get_addr());
	const std::vector<acl::string>& tokens = buf.split2(":");
	if (tokens.size() != 2)
	{
		printf("invalid master_addr: %s\r\n", node.get_addr());
		return false;
	}

	if (!redis.cluster_meet(tokens[0].c_str(), atoi(tokens[1].c_str())))
	{
		printf("cluster meet %s %s error: %s\r\n",
			tokens[0].c_str(), tokens[1].c_str(),
			redis.result_error());
		return false;
	}

	while (true)
	{
		if (cluster_meeting(redis, node.get_addr()) == true)
			break;
		acl_doze(meet_wait_);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

bool redis_builder::build_master(acl::redis_node& master)
{
	acl::redis_client client(master.get_addr());
	acl::redis redis(&client);

	if (master_set_slots(redis, master) == false)
		return false;

	const char* id = myself_id(redis);
	if (id == NULL || *id == 0)
	{
		printf("null id, master addr: %s\r\n", master.get_addr());
		return false;
	}
	master.set_id(id);

	printf("Build master: %s, %s\r\n", id, master.get_addr());

	const std::vector<acl::redis_node*>* slaves = master.get_slaves();
	std::vector<acl::redis_node*>::const_iterator cit;
	for (cit = slaves->begin(); cit != slaves->end(); ++cit)
	{
		if (add_slave(master, **cit) == false)
			return false;
	}
	printf("Build master OK\r\n");

	return true;
}

bool redis_builder::add_slave(const acl::redis_node& master,
	const acl::redis_node& slave)
{
	acl::redis_client client(slave.get_addr());
	acl::redis redis(&client);
	const char* master_addr = master.get_addr();
	if (master_addr == NULL || *master_addr == 0)
	{
		printf("master addr null\r\n");
		return false;
	}
	acl::string buf(master_addr);
	const std::vector<acl::string>& tokens = buf.split2(":");
	if (tokens.size() != 2)
	{
		printf("invalid master_addr: %s\r\n", master_addr);
		return false;
	}

	// CLUSTER MEET master node
	if (!redis.cluster_meet(tokens[0].c_str(), atoi(tokens[1].c_str())))
	{
		printf("cluster meet %s %s error: %s\r\n",
			tokens[0].c_str(), tokens[1].c_str(),
			redis.result_error());
		return false;
	}

	// wait for the master recognizing the slave
	while (true)
	{
		if (cluster_meeting(redis, master_addr) == true)
			break;
		acl_doze(meet_wait_);
	}

	if (redis.cluster_replicate(master.get_id()) == false)
	{
		printf("cluster replicate id: %s, error: %s, addr: %s\r\n",
			master.get_id(), redis.result_error(),
			slave.get_addr());
		return false;
	}

	return true;
}

acl::redis_node* redis_builder::find_slave(const acl::redis_node* node,
	const char* addr, size_t& nslaves)
{
	const std::vector<acl::redis_node*>* slaves = node->get_slaves();
	nslaves += slaves->size();
	std::vector<acl::redis_node*>::const_iterator cit;
	for (cit = slaves->begin(); cit != slaves->end(); ++cit)
	{
		if (strcasecmp((*cit)->get_addr(), addr) == 0)
			return *cit;
	}

	return NULL;
}

bool redis_builder::cluster_meeting(acl::redis& redis, const char* addr)
{
	acl::socket_stream* conn = redis.get_client()->get_stream();
	if (conn == NULL)
	{
		printf("connection disconnected!\r\n");
		return false;
	}
	const char* myaddr = conn->get_peer(true);
	const std::map<acl::string, acl::redis_node*>* nodes;
	if ((nodes = redis.cluster_nodes())== NULL)
	{
		printf("can't get cluster nodes, addr: %s\r\n", myaddr);
		return false;
	}

	size_t nslaves = 0;
	acl::redis_node* node = NULL;
	std::map<acl::string, acl::redis_node*>::const_iterator cit;
	for (cit = nodes->begin(); cit != nodes->end(); ++cit)
	{
		if (strcasecmp(cit->second->get_addr(), addr) == 0)
		{
			node = cit->second;
			break;
		}

		node = find_slave(cit->second, addr, nslaves);
		if (node != NULL)
			break;
	}

	if (node == NULL)
	{
		//show_nodes(nodes);

		time_t now = time(NULL);
		if (now - last_check_ >= 1)
		{
			printf("%s waiting for %s, nodes: %d, %d\r\n", myaddr,
				addr, (int) nodes->size(), (int) nslaves);
			last_check_ = now;
		}

		return false;
	}

	const char* type = node->get_type();
	if (strcasecmp(type, "slave") && strcasecmp(type, "master"))
	{
		time_t now = time(NULL);
		if (now - last_check_ >= 1)
		{
			printf("%s meeting with %s, status: %s\r\n",
				myaddr, addr, type);
			last_check_ = now;
		}

		return false;
	}

	printf("%s meet with %s OK, status: %s\r\n", myaddr, addr, type);

	return true;
}

bool redis_builder::master_set_slots(acl::redis& redis,
	acl::redis_node& master)
{
	const std::vector<std::pair<size_t, size_t> >& slots
		= master.get_slots();
	if (slots.size() != 1)
	{
		printf("invalid slots's size: %d, addr: %s\r\n",
			(int) slots.size(), master.get_addr());
		return false;
	}
	const std::pair<size_t, size_t> slot = slots[0];
	size_t min_slot = slot.first, max_slot = slot.second;
	size_t n = max_slot - min_slot + 1;
	int *slot_array = (int*) malloc(sizeof(int) * n);
	for (size_t i = 0; i < n; i++)
	{
		slot_array[i] = (int)min_slot;
		min_slot++;
	}

	if (redis.cluster_addslots(slot_array, n) == false)
	{
		printf("addslots error: %s, addr: %s, slots: %d, %d\r\n",
			redis.result_error(), master.get_addr(),
			(int) min_slot, (int) max_slot);
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

bool redis_builder::get_node_id(const char* addr, acl::string& node_id)
{
	acl::redis_client client(addr);
	acl::redis redis(&client);
	const char* ptr = myself_id(redis);
	if (ptr == NULL || *ptr == 0)
	{
		printf("null node_id from addr: %s\r\n", addr);
		return false;
	}

	node_id = ptr;
	return true;
}

const char* redis_builder::myself_id(acl::redis& redis)
{
	acl::socket_stream* conn = redis.get_client()->get_stream();
	if (conn == NULL)
	{
		printf("connection disconnected!\r\n");
		return NULL;
	}

	const char* addr = conn->get_peer(true);

	const std::map<acl::string, acl::redis_node*>* nodes =
		redis.cluster_nodes();
	if (nodes == NULL)
	{
		printf("cluster_nodes null, addr: %s\r\n", addr);
		return NULL;
	}

	std::map<acl::string, acl::redis_node*>::const_iterator it;

	for (it = nodes->begin(); it != nodes->end(); ++it)
	{
		const acl::redis_node* node = it->second;
		if (node->is_myself())
			return node->get_id();
	}

	printf("cluster_nodes no myself id, addr: %s\r\n", addr);

	return NULL;
}
