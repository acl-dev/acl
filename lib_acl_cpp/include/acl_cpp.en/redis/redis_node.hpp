#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <utility>
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

/**
 * This class is mainly used for redis_cluster command class to get information about cluster redis nodes
 * this class is mainly used for redis_cluster command class to
 * get some information about the nodes in redis cluster
 */
class ACL_CPP_API redis_node : public noncopyable {
public:
	/**
	 * When using this constructor to instantiate class object, need to call set_id and set_addr methods to set
	 * the unique identifier and service listening address of this redis node, and can also call other set_xxx setting methods
	 */
	redis_node();
	~redis_node();

	/**
	 * In addition to passing this node's ID identifier in constructor parameters, can also set through this function
	 * set the node's  ID
	 * @param id {const char*} Unique identifier of redis node in cluster
	 *  the unique ID for one redis node in the reids cluster
	 * @return {redis_node&}
	 */
	redis_node& set_id(const char* id);

	/**
	 * In addition to passing this node's address in constructor parameters, can also set through this function
	 * set the node's listening addr
	 * @param addr {const char*} Service address of redis node in cluster, format: ip:port
	 *  the listening addr of one redis node in the reids cluster
	 * @return {redis_node&}
	 */
	redis_node& set_addr(const char* addr);

	/**
	 * Set current node's type
	 * set the current node's type
	 * @param type {const char*}
	 * @return {redis_node&}
	 */
	redis_node& set_type(const char* type);

	/**
	 * Set whether current node is the current connection object
	 * set if the current node is belonging to the current connection
	 * @param yesno {bool}
	 * @return {redis_node&}
	 */
	redis_node& set_myself(bool yesno);

	/**
	 * When this node is a slave node, set current node's master node
	 * setting current slave node's master node
	 * @param master {const redis_node*} Master node object
	 *  the redis master node of the current slave in cluster
	 * @return {redis_node&}
	 */
	redis_node& set_master(const redis_node* master);

	/**
	 * Set current node is in handshaking stage
	 * set the current node being in handshaking status
	 * @param yesno {bool}
	 * @return {redis_node&}
	 */
	redis_node& set_handshaking(bool yesno);

	/**
	 * Set current node is in connected state
	 * set the node been connected in the cluster
	 * @param yesno {bool}
	 * @return {redis_node&}
	 */
	redis_node& set_connected(bool yesno);

	/**
	 * When this node is a slave node, set current node's master node identifier
	 * setting current node's master node when the node is slave node
	 * @param id {const char*} Master node unique identifier
	 *  the unique ID of the master node
	 * @return {redis_node&}
	 */
	redis_node& set_master_id(const char* id);

	/**
	 * When this node is a master node, add a slave node
	 * add one slave node to the current node if it's one master node
	 * @return {bool} Whether add was successful. Returns false when slave node already exists in current master node
	 *  false will be returned when the slave to be added is already
	 *  existing in the current master node
	 */
	bool add_slave(redis_node* slave);

	/**
	 * When this node is a master node, delete a slave node based on node unique identifier
	 * when the current node is a master node, this function will
	 * remove one slave node by the unique ID
	 * @param id {const char*} Redis node unique identifier
	 *  the unique ID of the redis node
	 * @return {const redis_node*} Returns deleted slave node. Returns NULL if does not exist
	 *  the slave node according to the ID will be returned, and if
	 *  not exists NULL will be returned
	 */
	redis_node* remove_slave(const char* id);

	/**
	 * When this node is a master node, clear all slave nodes of this node
	 * clear all the slave nodes in the current master node
	 * @param free_all {bool} Whether to release these slave nodes at the same time
	 *  if freeing the all slave nodes memory meanwhile
	 */
	void clear_slaves(bool free_all = false);

	/**
	 * When this node is a master node, add hash slot range
	 * add hash-slots range to the master node
	 * @param min {size_t} Start value of hash slot range
	 *  the begin hash-slot of the slots range
	 * @param max {size_t} End value of hash slot range
	 *  the end hash-slot of the slots range
	 */
	void add_slot_range(size_t min, size_t max);

	/**
	 * When this node is a master node, get master node's hash slot range. When it is a slave node, get its corresponding
	 * master node's hash slot range
	 * @return {const std::vector<std::pair<size_t, size_t> >&}
	 */
	const std::vector<std::pair<size_t, size_t> >& get_slots() const;

	/**
	 * Get current node's type
	 * get the node's type
	 * @return {const char*}
	 */
	const char* get_type() const {
		return type_.c_str();
	}

	/**
	 * Determine whether current node is the current connection object node
	 * check if the node belongs to the current connection
	 * @return {bool}
	 */
	bool is_myself() const {
		return myself_;
	}

	/**
	 * Determine whether current node is in handshaking stage
	 * check if the node is in handshaking status
	 * @return {bool}
	 */
	bool is_handshaking() const {
		return handshaking_;
	}

	/**
	 * Determine whether current node is already in connected state
	 * check if the node is connected in the cluster
	 * @return {bool}
	 */
	bool is_connected() const {
		return connected_;
	}

	/**
	 * When this node is a slave node, get this slave node's master node object
	 * get the current slave's master node
	 * @return {const redis_node*}
	 */
	const redis_node* get_master() const {
		return master_;
	}

	/**
	 * When this node is a slave node, get this slave node's corresponding master node's ID identifier
	 * when the current node is slave, getting its master's ID
	 * @return {const char*}
	 */
	const char* get_master_id() const {
		return master_id_.c_str();
	}

	/**
	 * When this node is a master node, get all slave nodes of this master node
	 * getting all the slaves of the master
	 * @return {const std::vector<redis_node*>*}
	 */
	const std::vector<redis_node*>* get_slaves() const {
		return &slaves_;
	}

	/**
	 * Determine whether current node is a master node in cluster
	 * check if the current node is a master in the redis cluster
	 * @return {bool}
	 */
	bool is_master() const {
		return master_ == this;
	}

	/**
	 * Get current node's ID identifier
	 * get the unique ID of the current node, set in constructor
	 * @return {const char*}
	 */
	const char* get_id() const {
		return id_.c_str();
	}

	/**
	 * Get current node's listening address
	 * get the listening addr of the current node, set in constructor
	 * @reutrn {const char*}
	 */
	const char* get_addr() const {
		return addr_.c_str();
	}

	/**
	 * result of CLUSTER NODES for redis.4.x.x:
	 * d52ea3cb4cdf7294ac1fb61c696ae6483377bcfc 127.0.0.1:16385@116385 master - 0 1428410625374 73 connected 5461-10922
	 * @return return 127.0.0.1:16385@116385 for redis.4.x.x
	 */
	const char* get_addr_info() const {
		return addr_info_.c_str();
	}

private:
	string id_;
	string addr_;
	string addr_info_;
	string type_;
	bool myself_;
	bool handshaking_;
	bool connected_;
	const redis_node* master_;
	string master_id_;
	std::vector<redis_node*> slaves_;
	std::vector<std::pair<size_t, size_t> > slots_;
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

