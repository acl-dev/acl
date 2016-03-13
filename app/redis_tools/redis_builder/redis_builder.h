#pragma once
#include <vector>
#include <map>

class redis_builder
{
public:
	redis_builder(const char* passwd = NULL, int meet_wait = 100);
	~redis_builder(void);

	bool build(const char* conf, size_t replicas, bool just_display);

	bool add_node(const char* addr, const char* new_node_addr, bool slave);
	bool del_node(const char* addr, const char* node_id);

private:
	acl::string passwd_;
	int meet_wait_;
	std::vector<acl::redis_node*> masters_;
	time_t last_check_;

	// load the cluster.xml configure and create redis nodes for creating
	bool load(const char* conf, size_t replicas);

	// parse xml and create cluster nodes before connecting them
	bool create_cluster(acl::xml& xml);

	// parse xml and create cluster nodes before connecting them,
	// don't use the master/slave relation from xml configure,
	// use the replicas param to build the cluster automatic
	bool create_cluster(acl::xml& xml, size_t replicas);

	// peek one master node and remove it from nodes
	acl::redis_node* peek_master(std::vector<acl::redis_node*>& nodes,
		std::map<acl::string, size_t>& addrs);

	// peek one slave node and remove it from nodes
	acl::redis_node* peek_slave(const char* master_addr,
		std::vector<acl::redis_node*>& nodes,
		std::map<acl::string, size_t>& addrs);

	// create one master node according to one xml node of configure
	acl::redis_node* create_master(acl::xml_node& node);

	// create one node
	acl::redis_node* create_node(acl::xml_node& node);

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
