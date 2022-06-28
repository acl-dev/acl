#pragma once
#include <map>

class redis_util
{
public:
	redis_util(void);
	~redis_util(void);

	// get the node's id of the given addr
	static bool get_node_id(const char* addr, acl::string& node_id,
		const char* passwd);

	// get the current node's ID
	static bool get_node_id(acl::redis& redis, acl::string& node_id);

	// get ip from the addr which format is ip:port
	static bool get_ip(const char* addr, acl::string& ip);

	static bool addr_split(const char* addr, acl::string& ip, int& port);

	// show the nodes's information, including master and slave
	static void print_nodes(int nested,
		const std::vector<acl::redis_node*>& nodes);

	// free all nodes nestly
	static void free_nodes(const std::vector<acl::redis_node*>& nodes);

	// sorting the givent redis_nodes by which machine ip
	static void sort(const std::map<acl::string, acl::redis_node*>& in,
		std::map<acl::string, std::vector<acl::redis_node*>* >& out);

	// free vector holding redis_nodes
	static void clear_nodes_container(
		std::map<acl::string, std::vector<acl::redis_node*>* >& nodes);

	// get all the masters of cluster
	static const std::map<acl::string, acl::redis_node*>*
		get_masters(acl::redis&);

	// get all the slaves of cluster
	static void get_slaves(acl::redis&,
		std::vector<const acl::redis_node*>&);

	// get nodes formed bye one node maybe master or slave of the cluster
	static void get_nodes(acl::redis& redis, bool prefer_master,
		std::vector<acl::redis_node*>& nodes);

	// get all the nodes of the cluster
	static void get_all_nodes(acl::redis& redis,
		std::vector<const acl::redis_node*>& nodes);

private:
	static const std::map<acl::string, acl::redis_node*>*
		get_masters2(acl::redis&);

	static void add_slaves(const acl::redis_node* node,
		std::vector<const acl::redis_node*>& nodes);
};
