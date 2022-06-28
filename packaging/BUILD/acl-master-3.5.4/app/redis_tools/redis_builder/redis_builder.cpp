#include "stdafx.h"
#include "redis_status.h"
#include "redis_util.h"
#include "redis_builder.h"

#define MAX_SLOTS 16384

redis_builder::redis_builder(const char* passwd /* = NULL */, int meet_wait /* = 100 */)
: meet_wait_(meet_wait)
, last_check_(0)
{
	if (passwd && *passwd) {
		passwd_ = passwd;
	}
}

redis_builder::~redis_builder(void)
{
	redis_util::free_nodes(masters_);
}

//////////////////////////////////////////////////////////////////////////

bool redis_builder::add_node(const char* addr,
	const char* new_node_addr, bool slave)
{
	acl::redis_client client(new_node_addr);
	client.set_password(passwd_);
	acl::redis redis(&client);

	acl::string buf(addr);
	const std::vector<acl::string>& tokens = buf.split2(":");
	if (tokens.size() != 2) {
		printf("%s: invalid addr: %s\r\n", __FUNCTION__, addr);
		return false;
	}

	// CLUSTER MEET master node
	if (!redis.cluster_meet(tokens[0].c_str(), atoi(tokens[1].c_str()))) {
		printf("%s: cluster meet %s %s error: %s\r\n", __FUNCTION__,
			tokens[0].c_str(), tokens[1].c_str(),
			redis.result_error());
		return false;
	}

	// wait for the master recognizing the slave
	while (true) {
		if (cluster_meeting(redis, addr)) {
			break;
		}
		acl_doze(meet_wait_);
	}

	if (!slave) {
		return true;
	}

	acl::string node_id;
	if (!redis_util::get_node_id(addr, node_id, passwd_)) {
		printf("%s: can't get master(%s)'s node_id\r\n",
			__FUNCTION__, addr);
		return false;
	}

	if (!redis.cluster_replicate(node_id.c_str())) {
		printf("%s: cluster replicate id: %s, error: %s, "
			"master_addr: %s, slave_addr: %s\r\n",
			__FUNCTION__, node_id.c_str(), redis.result_error(),
			addr, new_node_addr);
		return false;
	}

	return true;
}

bool redis_builder::del_node(const char* addr, const char* node_id)
{
	acl::redis_client client(addr);
	client.set_password(passwd_);
	acl::redis redis(&client);
	if (!redis.cluster_forget(node_id)) {
		printf("%s: del node: %s error: %s, addr: %s\r\n",
			__FUNCTION__, node_id, redis.result_error(), addr);
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////

bool redis_builder::build(const char* conf, size_t replicas, bool just_display)
{
	if (!load(conf, replicas)) {
		return false;
	}

	if (masters_.empty()) {
		printf("%s: no nodes available\r\n", __FUNCTION__);
		return false;
	}

	printf("===================================================\r\n");

	redis_util::print_nodes(0, masters_);

	if (just_display) {
		return true;
	}

	return build_cluster();
}

bool redis_builder::load(const char* conf, size_t replicas)
{
	acl::string buf;

	// load xml information from local file
	if (!acl::ifstream::load(conf, &buf)) {
		printf("%s: load configure error: %s, file: %s\r\n",
			__FUNCTION__, acl::last_serror(), conf);
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
	acl::xml1 xml(buf.c_str());

	if (replicas > 0) {
		return create_cluster(xml, replicas);
	} else {
		return create_cluster(xml);
	}
}

bool redis_builder::create_cluster(acl::xml& xml, size_t replicas)
{
	const char* tag = "node";
	const std::vector<acl::xml_node*>& nodes = xml.getElementsByTagName(tag);

	if (nodes.empty()) {
		printf("%s: nodes null\r\n", __FUNCTION__);
		return false;
	}

	std::vector<acl::redis_node*> redis_nodes;

	std::vector<acl::xml_node*>::const_iterator cit;
	for (cit = nodes.begin(); cit != nodes.end(); ++cit) {
		acl::redis_node* n = create_node(**cit);
		if (n != NULL) {
			redis_nodes.push_back(n);
		}
	}

	size_t mod = redis_nodes.size() % (replicas + 1);
	if (replicas > 0 && mod != 0) {
		printf("%s: nodes' size(%d) %% replicas + 1(%d) is %d != 0\r\n",
			__FUNCTION__, (int) redis_nodes.size(),
			(int) replicas + 1, (int) mod);
		std::vector<acl::redis_node*>::iterator it;
		for (it = redis_nodes.begin(); it != redis_nodes.end(); ++it) {
			delete *it;
		}
		return false;
	}

	std::map<acl::string, size_t> master_addrs;
	std::map<acl::string, size_t> all_addrs;
	acl::redis_node* master, *slave;
	acl::string ip;
	while (true) {
		master = peek_master(redis_nodes, master_addrs);
		if (master == NULL) {
			break;
		}
		masters_.push_back(master);

		assert(redis_util::get_ip(master->get_addr(), ip));
		all_addrs[ip]++;

		for (size_t i = 0; i < replicas; i++) {
			slave = peek_slave(master->get_addr(),
					redis_nodes, all_addrs);
			if (slave != NULL) {
				master->add_slave(slave);
			}
		}
	}

	return true;
}

acl::redis_node* redis_builder::peek_master(std::vector<acl::redis_node*>& nodes,
	std::map<acl::string, size_t>& addrs)
{
	// at first, find the addr not be as a master
	acl::string ip;
	std::vector<acl::redis_node*>::iterator it, next;
	for (it = nodes.begin(), next = it; it != nodes.end(); it = next) {
		++next;
		const char* addr = (*it)->get_addr();
		if (!redis_util::get_ip(addr, ip)) {
			printf("%s: delete invalid addr: %s\r\n",
				__FUNCTION__, addr);
			delete *it;
			nodes.erase(it);
			continue;
		}
		if (addrs.find(ip) != addrs.end()) {
			continue;
		}
		acl::redis_node* master = *it;
		addrs[ip]++;
		nodes.erase(it);
		return master;
	}

	// then, find the addr in which masters are the least
	size_t n = 100000;
	acl::string min_addr;
	std::vector<acl::redis_node*>::iterator min_node = nodes.end();
	std::map<acl::string, size_t>::iterator iter_addr;
	for (it = nodes.begin(); it != nodes.end(); ++it) {
		const char* addr = (*it)->get_addr();
		assert(redis_util::get_ip(addr, ip));
		iter_addr = addrs.find(ip);
		assert(iter_addr != addrs.end());
		if (iter_addr->second <= n) {
			n = iter_addr->second;
			min_addr = ip;
			min_node = it;
		}
	}

	if (min_addr.empty() || min_node == nodes.end()) {
		return NULL;
	}

	addrs[min_addr]++;
	acl::redis_node* master = *min_node;
	nodes.erase(min_node);
	return master;
}

acl::redis_node* redis_builder::peek_slave(const char* master_addr,
	std::vector<acl::redis_node*>& nodes,
	std::map<acl::string, size_t>& addrs)
{
	acl::string master_ip;
	assert(redis_util::get_ip(master_addr, master_ip));

	acl::string ip;
	std::vector<acl::redis_node*> slaves;
	std::vector<acl::redis_node*>::iterator it, next;
	for (it = nodes.begin(), next = it; it != nodes.end(); it = next) {
		++next;
		const char* addr = (*it)->get_addr();
		if (!redis_util::get_ip(addr, ip)) {
			printf("%s: delete invalid addr: %s\r\n",
				__FUNCTION__, addr);
			delete *it;
			nodes.erase(it);
			continue;
		}
		if (ip == master_ip) {
			continue;
		}
		slaves.push_back(*it);
	}

	if (slaves.empty()) {
		//printf("not found valid slave node for master: %s\r\n",
		//	master_addr);
		for (it = nodes.begin(); it != nodes.end(); ++it)
			slaves.push_back(*it);
	}

	size_t n = 100000;
	acl::string min_addr;
	acl::redis_node* slave = NULL;

	std::map<acl::string, size_t>::iterator iter_addr;
	for (it = slaves.begin(); it != slaves.end(); ++it) {
		const char* addr = (*it)->get_addr();
		assert(redis_util::get_ip(addr, ip));
		iter_addr = addrs.find(ip);
		if (iter_addr == addrs.end()) {
			min_addr = ip;
			slave = *it;
			break;
		}
		if (iter_addr->second <= n) {
			n = iter_addr->second;
			min_addr = ip;
			slave = *it;
		}
	}

	if (min_addr.empty() || slave == NULL) {
		return NULL;
	}

	addrs[min_addr]++;
	bool found = false;

	for (it = nodes.begin(); it != nodes.end(); ++it) {
		if (*it == slave) {
			nodes.erase(it);
			found = true;
			break;
		}
	}

	assert(found);
	return slave;
}

bool redis_builder::create_cluster(acl::xml& xml)
{
	// get the master redis nodes
	const char* tags = "xml/node";
	const std::vector<acl::xml_node*>& nodes = xml.getElementsByTags(tags);

	if (nodes.empty()) {
		printf("%s: nodes null\r\n", __FUNCTION__);
		return false;
	}

	// iterate all the master nodes including their's slaves
	std::vector<acl::xml_node*>::const_iterator cit;
	for (cit = nodes.begin(); cit != nodes.end(); ++cit) {
		acl::redis_node* master = create_master(**cit);
		if (master != NULL) {
			masters_.push_back(master);
		}
	}

	return true;
}

acl::redis_node* redis_builder::create_master(acl::xml_node& node)
{
	const char* addr = node.attr_value("addr");
	if (addr == NULL || *addr == 0) {
		printf("%s: no addr in the master node\r\n", __FUNCTION__);
		return NULL;
	}

	//printf(">>> master: %s\r\n", addr);

	acl::redis_node* master = new acl::redis_node;
	master->set_addr(addr);

	// iterate all the slaves of the master, and add them to master
	acl::xml_node* child = node.first_child();
	while (child != NULL) {
		acl::redis_node* slave = create_node(*child);
		if (slave != NULL) {
			master->add_slave(slave);
		}
		child = node.next_child();
	}

	//const std::vector<acl::redis_node*>* slaves = master->get_slaves();
	//printf(">>>slave's size: %d\r\n", (int) slaves->size());
	return master;
}

acl::redis_node* redis_builder::create_node(acl::xml_node& node)
{
	const char* addr = node.attr_value("addr");
	if (addr == NULL || *addr == 0) {
		printf("%s: no addr in the node\r\n", __FUNCTION__);
		return NULL;
	}

	//printf("%s: node: %s\r\n", __FUNCTION__, addr);
	acl::redis_node* slave = new acl::redis_node;
	slave->set_addr(addr);
	return slave;
}

bool redis_builder::build_cluster()
{
	if (masters_.empty()) {
		printf("%s: no master available!\r\n", __FUNCTION__);
		return false;
	}

	size_t range = MAX_SLOTS / masters_.size();
	size_t begin = 0, end = MAX_SLOTS % masters_.size() + range -1;

	// build every master node, and connect all of its slaves.

	std::vector<acl::redis_node*>::iterator it;
	for (it = masters_.begin(); it != masters_.end(); ++it) {
		if (it != masters_.begin()) {
			printf("----------------------------------------\r\n");
		}

		(*it)->add_slot_range(begin, end);
		if (!build_master(**it)) {
			return false;
		}
		begin = end + 1;
		end = end + range;
	}

	it = masters_.begin();
	acl::redis_client client((*it)->get_addr());
	client.set_password(passwd_);
	acl::redis master(&client);

	// let one master to connect all other master nodes

	printf("===================================================\r\n");
	printf("Meeting all masters and slaves ...\r\n");

	std::vector<acl::redis_node*> all_slaves;
	std::vector<acl::redis_node*>::const_iterator cit;

	for (++it; it != masters_.end(); ++it) {
		if (!cluster_meet(master, **it)) {
			return false;
		}
		const std::vector<acl::redis_node*>* slaves = (*it)->get_slaves();
		for (cit = slaves->begin(); cit != slaves->end(); ++cit) {
			all_slaves.push_back(*cit);
		}
	}

	while (true) {
		int nwait = 0;
		for (cit = all_slaves.begin(); cit != all_slaves.end(); ++cit) {
			if ((*cit)->is_connected()) {
				continue;
			}
			if (!cluster_meeting(master, (*cit)->get_addr())) {
				nwait++;
			} else {
				(*cit)->set_connected(true);
			}
		}
		if (nwait == 0) {
			break;
		}
		acl_doze(meet_wait_);
	}

	/////////////////////////////////////////////////////////////////////

	printf("===================================================\r\n");
	printf("All nodes of cluster:\r\n");

	const std::map<acl::string, acl::redis_node*>* nodes;
	if ((nodes = master.cluster_nodes())== NULL) {
		printf("%s: can't get cluster nodes, addr: %s\r\n",
			__FUNCTION__, client.get_stream()->get_peer(true));
		return false;
	}

#ifdef ACL_UNIX
	redis_status::show_nodes_tree(*nodes);
#else
	redis_status::show_nodes(nodes);
#endif
	return true;
}

bool redis_builder::cluster_meet(acl::redis& redis, const acl::redis_node& node)
{
	acl::string buf(node.get_addr());
	const std::vector<acl::string>& tokens = buf.split2(":");
	if (tokens.size() != 2) {
		printf("%s: invalid addr: %s\r\n",
			__FUNCTION__, node.get_addr());
		return false;
	}

	if (!redis.cluster_meet(tokens[0].c_str(), atoi(tokens[1].c_str()))) {
		printf("%s: cluster meet %s %s error: %s\r\n",
			__FUNCTION__, tokens[0].c_str(), tokens[1].c_str(),
			redis.result_error());
		return false;
	}

	while (true) {
		if (cluster_meeting(redis, node.get_addr())) {
			break;
		}
		acl_doze(meet_wait_);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

bool redis_builder::build_master(acl::redis_node& master)
{
	acl::redis_client client(master.get_addr());
	client.set_password(passwd_);
	acl::redis redis(&client);

	if (!master_set_slots(redis, master)) {
		return false;
	}

	acl::string id;
	if (!redis_util::get_node_id(redis, id)) {
		printf("%s: null id, master addr: %s\r\n",
			__FUNCTION__, master.get_addr());
		return false;
	}
	master.set_id(id.c_str());

	printf("Build master: %s, %s\r\n", id.c_str(), master.get_addr());

	const std::vector<acl::redis_node*>* slaves = master.get_slaves();
	std::vector<acl::redis_node*>::const_iterator cit;
	for (cit = slaves->begin(); cit != slaves->end(); ++cit) {
		if (!add_slave(master, **cit)) {
			return false;
		}
	}

	printf("Build master OK\r\n");
	return true;
}

bool redis_builder::add_slave(const acl::redis_node& master,
	const acl::redis_node& slave)
{
	acl::redis_client client(slave.get_addr());
	client.set_password(passwd_);
	acl::redis redis(&client);
	const char* master_addr = master.get_addr();
	if (master_addr == NULL || *master_addr == 0) {
		printf("%s: master addr null\r\n", __FUNCTION__);
		return false;
	}
	acl::string buf(master_addr);
	const std::vector<acl::string>& tokens = buf.split2(":");
	if (tokens.size() != 2) {
		printf("%s: invalid master_addr: %s\r\n",
			__FUNCTION__, master_addr);
		return false;
	}

	// CLUSTER MEET master node
	if (!redis.cluster_meet(tokens[0].c_str(), atoi(tokens[1].c_str()))) {
		printf("%s: cluster meet %s %s error: %s\r\n",
			__FUNCTION__, tokens[0].c_str(), tokens[1].c_str(),
			redis.result_error());
		return false;
	}

	// wait for the master recognizing the slave
	while (true) {
		if (cluster_meeting(redis, master_addr)) {
			break;
		}
		acl_doze(meet_wait_);
	}

	if (!redis.cluster_replicate(master.get_id())) {
		printf("%s: cluster replicate id: %s, error: %s, addr: %s\r\n",
			__FUNCTION__, master.get_id(), redis.result_error(),
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
	for (cit = slaves->begin(); cit != slaves->end(); ++cit) {
		if (strcasecmp((*cit)->get_addr(), addr) == 0) {
			return *cit;
		}
	}

	return NULL;
}

bool redis_builder::cluster_meeting(acl::redis& redis, const char* addr)
{
	acl::socket_stream* conn = redis.get_client()->get_stream();
	if (conn == NULL) {
		printf("%s: connection disconnected!\r\n", __FUNCTION__);
		return false;
	}
	const char* myaddr = conn->get_peer(true);
	const std::map<acl::string, acl::redis_node*>* nodes;
	if ((nodes = redis.cluster_nodes())== NULL) {
		printf("%s: can't get cluster nodes, addr: %s\r\n",
			__FUNCTION__, myaddr);
		return false;
	}

	size_t nslaves = 0;
	acl::redis_node* node = NULL;
	std::map<acl::string, acl::redis_node*>::const_iterator cit;
	for (cit = nodes->begin(); cit != nodes->end(); ++cit) {
		if (strcasecmp(cit->second->get_addr(), addr) == 0) {
			node = cit->second;
			break;
		}

		node = find_slave(cit->second, addr, nslaves);
		if (node != NULL) {
			break;
		}
	}

	if (node == NULL) {
		//show_nodes(nodes);

		time_t now = time(NULL);
		if (now - last_check_ >= 1) {
			printf("%s waiting for %s, nodes: %d, %d\r\n", myaddr,
				addr, (int) nodes->size(), (int) nslaves);
			last_check_ = now;
		}

		return false;
	}

	const char* type = node->get_type();
	if (strcasecmp(type, "slave") && strcasecmp(type, "master")) {
		time_t now = time(NULL);
		if (now - last_check_ >= 1) {
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
	if (slots.size() != 1) {
		printf("%s: invalid slots's size: %d, addr: %s\r\n",
			__FUNCTION__, (int) slots.size(), master.get_addr());
		return false;
	}
	const std::pair<size_t, size_t> slot = slots[0];
	size_t min_slot = slot.first, max_slot = slot.second;
	size_t n = max_slot - min_slot + 1;
	int *slot_array = (int*) malloc(sizeof(int) * n);
	for (size_t i = 0; i < n; i++) {
		slot_array[i] = (int)min_slot;
		min_slot++;
	}

	if (!redis.cluster_addslots(slot_array, n)) {
		printf("%s: addslots error: %s, addr: %s, slots: %d, %d\r\n",
			__FUNCTION__, redis.result_error(), master.get_addr(),
			(int) min_slot, (int) max_slot);
		return false;
	}
	return true;
}
