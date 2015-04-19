#pragma once
#include <vector>
#include <map>

class redis_builder
{
public:
	redis_builder(int meet_wait = 100);
	~redis_builder(void);

	bool build(const char* conf);

	bool add_node(const char* addr, const char* new_node_addr, bool slave);
	bool del_node(const char* addr, const char* node_id);

	// get the node's id of the given addr
	bool get_node_id(const char* addr, acl::string& node_id);
	// get the current node's ID
	const char* myself_id(acl::redis& redis);

private:
	int meet_wait_;
	std::vector<acl::redis_node*> masters_;
	time_t last_check_;

	// load the cluster.xml configure and create redis nodes for creating
	bool load(const char* conf);

	// create one master node according to one xml node of configure
	acl::redis_node* create_master(acl::xml_node& node);

	// create one slave node
	acl::redis_node* create_slave(acl::xml_node& node);

	// begin build the redis cluster, connect all redis nodes
	bool build_cluster();

	// allocate slots for every master, and let its slaves connect
	// to their master node
	bool build_master(acl::redis_node& master);

	// let one node meet to another one
	bool cluster_meet(acl::redis& redis, const acl::redis_node& node);

	// check the MEET status between the current node and the other one
	bool cluster_meeting(acl::redis& redis, const char* addr);

	// add slots to one master node
	bool master_set_slots(acl::redis& redis, acl::redis_node& master);

	// add one slave to its master node, let the slave MEET its master,
	// and make the slave REPLICATE its master.
	bool add_slave(const acl::redis_node& master,
		const acl::redis_node& slave);

	// check if the given addr was in the node's slave
	acl::redis_node* find_slave(const acl::redis_node* node,
		const char* addr, size_t& nslaves);
};
