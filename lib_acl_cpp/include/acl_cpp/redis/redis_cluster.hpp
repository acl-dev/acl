#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <list>
#include <map>
#include "../stdlib/string.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

class redis_result;
class redis_node;
class redis_slot;

class ACL_CPP_API redis_cluster : virtual public redis_command {
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_cluster();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	explicit redis_cluster(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	explicit redis_cluster(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_cluster(redis_client_cluster* cluster, size_t max_conns);

	explicit redis_cluster(redis_client_pipeline* pipeline);

	virtual ~redis_cluster();

	/**
	 * Batch add available hash slots. Must end with hash slot value < 0 to
	 * indicate end
	 * add some hash-slots, the last slot value must be < 0 indicating
	 * the end of the slots array
	 * @param first {int} First hash slot. This value must be >= 0 to be effective
	 *  the first hash-slot which must be >= 0
	 *  param slot_list {const int[]} List of hash slots to add
	 *  the hash-slots array to be added
	 *  param n {size_t} the count of the hash-slots list
	 *  param slot_list {const std::vector<init>&} List of hash slots to add
	 *  the hash-slots list to be added
	 * @return {bool} Whether successful
	 *  return true if successful
	 */
	bool cluster_addslots(int first, ...);
	bool cluster_addslots(const int slot_list[], size_t n);
	bool cluster_addslots(const std::vector<int>& slot_list);

	/**
	 * Batch delete hash slots. Must end with hash slot < 0 to indicate end
	 * remove some hash-slots, the last slot value must be < 0 indicating
	 * the end of the slots array
	 * @param first {int} First hash slot. This value must be >= 0 to be effective
	 *  the first hash-slot which must be >= 0
	 *  param slot_list {const int[]} List of hash slots to delete
	 *  the hash-slots array to be removed
	 *  param n {size_t} the count of the hash-slots list
	 *  param slot_list {const std::vector<init>&} List of hash slots to delete
	 *  the hash-slots array to be removed
	 * @return {bool} Whether successful
	 *  return true if successful
	 */
	bool cluster_delslots(int first, ...);
	bool cluster_delslots(const int slot_list[], size_t n);
	bool cluster_delslots(const std::vector<int>& slot_list);

	/**
	 * Get key name collection of objects currently stored in a hash slot
	 * get keys array stored in one specified hash-slot
	 * @param slot {size_t} Hash slot value
	 *  the specified hash-slot
	 * @param max {size_t} Limit on number of result set
	 *  limit the max results count
	 * @param result {std::list<acl::string>&} Store result set
	 *  stored the results
	 * @return {int} Number of query result set. -1 indicates error
	 *  >= 0 if OK, -1 if error
	 */
	int cluster_getkeysinslot(size_t slot, size_t max, std::list<string>& result);

	/**
	 * When building redis cluster, can use this command to let a redis node
	 * connect to another node
	 * let one redis node to link to the other redis node
	 * when buiding the redis cluster
	 * @param ip {const char*} IP address of another redis node to be connected
	 *  the other redis node's ip to be linked
	 * @param port {int} Port of another redis node to be connected
	 *  the other redis node's port to be linked
	 * @return {bool} Whether connection was successful
	 *  if the linking is successful
	 */
	bool cluster_meet(const char* ip, int port);

	/**
	 * Reset a redis node's state, making it leave cluster nodes, clear hash
	 * slot-node correspondence.
	 * This method is equivalent to reset_soft
	 * reset one redis node's status, escaping from the other nodes
	 * of the redis cluster, and clearing slot-to-nodes mapping;
	 * same as reset_soft
	 * @return {bool} Whether operation was successful
	 *  if the operation is successful
	 */
	bool cluster_reset();
	bool cluster_reset_hard();
	bool cluster_reset_soft();

	/**
	 * Set a hash slot on current redis node to be in importing state
	 * set the hash-slot in importing status from the other node
	 * to the current node
	 * @param slot {size_t} Hash slot value
	 *  the hash-slot value
	 * @param src_node {const char*} Redis source node of this hash slot
	 *  the source redis-node of the hash-slot importing from
	 * @return {boo} Whether setting status was successful
	 *  if success for setting the slot's status
	 */
	bool cluster_setslot_importing(size_t slot, const char* src_node);

	/**
	 * Set a hash slot on current redis node to be in migrating state
	 * set the hash-slot in migrating status to the other node
	 * from the current node
	 * @param slot {size_t} Hash slot value
	 *  the hash-slot value
	 * @param dst_node {const char*} Redis migration target node of this hash slot
	 *  the target redis-node of the hash-slot migrating to
	 * @return {boo} Whether setting status was successful
	 *  if success for setting the slot's status
	 */
	bool cluster_setslot_migrating(size_t slot, const char* dst_node);

	/**
	 * After importing/migrating hash slot completes, use this operation to set
	 * hash slot to stable state
	 * set the hash-slot stable after importing or migrating
	 * @param slot {size_t} Hash slot value
	 *  the hash-slot value
	 * @return {bool} Whether setting status was successful
	 *  if success for setting the slot's status
	 */
	bool cluster_setslot_stable(size_t slot);

	/**
	 * Set specified hash slot to a specified redis node. This command has
	 * relatively complex behavioral characteristics. For details,
	 * please refer to official online documentation
	 * set one hash-slot to one redis node, for more help see online doc
	 * @param slot {size_t} Hash slot value
	 *  the hash-slot to be set
	 * @param node {const char*} Redis node receiving this hash slot
	 *  the redis node to be holding the hash-slot
	 * @return {bool} Whether operation was successful
	 *  if the operation is successful
	 */
	bool cluster_setslot_node(size_t slot, const char* node);

	/**
	 * Get number of error reports from a specified redis node
	 * get the count of the failure resports by one redis node
	 * @param node {const char*} A specified redis node
	 * @return {int} Number of node error reports. Normally >= 0. If return value
	 * is -1, it indicates operation error
	 *  return the failure count reporting by the specified redis node,
	 *  return value >= 0 if successful, or -1 for error happened
	 */
	int cluster_count_failure_reports(const char* node);

	/**
	 * This command operation can only be sent to a slave node for failover
	 * of master node, making current slave become master. Needs to negotiate with
	 * its master node, and needs approval from other master nodes
	 * this command can only be sent to one slave node for failover
	 * of the master node, make the current slave to be the master
	 * @return {bool} Whether operation was successful
	 *  if the operation is successful
	 */
	bool cluster_failover();

	/**
	 * Forcefully change a slave node to master node. This operation does not need
	 * to negotiate with original master node, but still needs
	 * approval from majority of master nodes in cluster
	 * force a slave to be the master, not handshake with it's master,
	 * but still need get agreement by majority of the masters in cluster
	 * @return {bool} Whether operation was successful
	 *  if the operation is successful
	 */
	bool cluster_failover_force();

	/**
	 * Forcefully change a slave node to master node. This operation does not need
	 * to negotiate with original master node and other master nodes
	 * in cluster
	 * force a slave to be the master, not handshake with it's master,
	 * and also no need get agreement by the other masters in cluster
	 * @return {bool} Whether operation was successful
	 *  if the operation is successful
	 */
	bool cluster_failover_takeover();

	/**
	 * Get some overview information about current cluster
	 * get running informantion about the redis cluster
	 * @param result {std::map<acl::string, acl::string>&} Store result
	 *  store the result of this operation
	 * @return {bool} Whether operation was successful
	 *  if this operation is successful
	 */
	bool cluster_info(std::map<string, string>& result);

	/**
	 * Let current redis node save configuration information to nodes.conf on disk
	 * let the current node to save the cluster information in nodes.conf
	 * @return {bool} Whether operation was successful
	 *  if this operation is successful
	 */
	bool cluster_saveconfig();

	/**
	 * Get total number of objects in a hash slot
	 * get all the keys's count in one hash-slot
	 * @param slot {size_t} Specified hash slot
	 *  the specified hash-slot
	 * @return {int} Returns number of objects in hash slot. -1 indicates error
	 *　return the keys's count in the hash-slot, return -1 if error 
	 */
	int cluster_countkeysinslot(size_t slot);

	/**
	 * Remove specified node from current node
	 * remove the specified node from the current node
	 * @param node {const char*} Specified node to be removed
	 *  the speicied node to be removed
	 * @return {bool} Whether operation was successful
	 *  if this operation is successful
	 */
	bool cluster_forget(const char* node);

	/**
	 * Get hash slot that a key belongs to
	 * get the hash-slot wich the key belongs to
	 * @param key {const char*} Key value
	 *  the key string
	 * @return {int} Hash slot value. >= 0 indicates success, -1 indicates
	 * operation failed
	 *  return the key's hash-slot, >= 0 if successful, -1 on error
	 */
	int cluster_keyslot(const char* key);

	/**
	 * Set specified node as slave node. If this node was originally a slave node,
	 * will also return success
	 * set the specified node to be a slave node
	 * @param node {const char*} Specified node identifier
	 *  the specified node
	 * @return {bool} Whether operation was successful
	 *  if this operation is successful
	 */
	bool cluster_replicate(const char* node);

	bool cluster_set_config_epoch(const char* epoch);

	/**
	 * Get distribution of all hash slots across various redis nodes in cluster
	 * get all nodes with all slots
	 * @return {const std::vector<redis_slot*>*} Returns collection of all master
	 * nodes storing hash slot information.
	 *  Returns NULL indicates error
	 *  return all the master nodes with all hash-slots in them,
	 *  and NULL will be returned if error happened, and the return
	 *  value needn't be freed because it can be freed internal
	 */
	const std::vector<redis_slot*>* cluster_slots();
	
	/**
	 * Get all master nodes in current cluster. All slave nodes of master nodes can
	 * be obtained through
	 * redis_node::get_slaves
	 * get all the masters of the cluster, and master's slave nodes
	 * can be got by redis_node::get_slaves
	 * @return {const std::map<string, redis_node*>*} Returns NULL indicates error
	 *  return NULL if error happened, the return value needn't be
	 *  freed because it can be freed internal
	 */
	const std::map<string, redis_node*>* cluster_nodes();

	/**
	 * Get all slave nodes of specified master node
	 * get all slave nodes of the specified master node
	 * @return node {const char*} Master node identifier. Returns NULL indicates
	 * error. This
	 * return result does not need to be freed, internally automatically maintained
	 *  one of the master node, NULL if error, and the return value
	 *  needn't be freed because it can be freed internal
	 */
	const std::vector<redis_node*>* cluster_slaves(const char* node);

private:
	std::vector<redis_slot*> slots_;

	redis_slot* get_slot_master(const redis_result* rr);
	redis_slot* get_slot(const redis_result* rr,
		size_t slot_max, size_t slot_min);
	void free_slots();

private:
	std::map<string, redis_node*> masters_;

	redis_node* get_node(string& line);
	void add_slot_range(redis_node* node, char* slots);
	void free_masters();
	redis_node* get_slave(const std::vector<string>& tokens);

private:
	std::vector<redis_node*> slaves_;
	void free_slaves();
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

