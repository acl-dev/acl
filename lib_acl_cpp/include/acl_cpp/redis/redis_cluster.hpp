#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <list>
#include <map>
#include "../stdlib/string.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

class redis_result;
class redis_node;
class redis_slot;

class ACL_CPP_API redis_cluster : virtual public redis_command
{
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_cluster(void);

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_cluster(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	redis_cluster(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_cluster(redis_client_cluster* cluster, size_t max_conns);

	virtual ~redis_cluster(void);

	/**
	 * 批量添加可用的哈希槽，最后必须以小于 0 的哈希槽值表示结束
	 * add some hash-slots, the last slot value must be < 0 indicating
	 * the end of the slots array
	 * @param first {int} 第一个哈希槽，该值必须 >= 0 才有效
	 *  the first hash-slot which must be >= 0
	 *  param slot_list {const int[]} 要添加的哈希槽的列表
	 *  the hash-slots array to be added
	 *  param n {size_t} the count of the hash-slots list
	 *  param slot_list {const std::vector<init>&} 要添加的哈希槽的列表
	 *  the hash-slots list to be added
	 * @return {bool} 是否成功
	 *  return true if successful
	 */
	bool cluster_addslots(int first, ...);
	bool cluster_addslots(const int slot_list[], size_t n);
	bool cluster_addslots(const std::vector<int>& slot_list);

	/**
	 * 批量删除哈希槽，最后必须以小于 0 的哈希槽表示结束
	 * remove some hash-slots, the last slot value must be < 0 indicating
	 * the end of the slots array
	 * @param first {int} 第一个哈希槽，该值必须 >= 0 才有效
	 *  the first hash-slot which must be >= 0
	 *  param slot_list {const int[]} 要删除的哈希槽的列表
	 *  the hash-slots array to be removed
	 *  param n {size_t} the count of the hash-slots list
	 *  param slot_list {const std::vector<init>&} 要删除的哈希槽的列表
	 *  the hash-slots array to be removed
	 * @return {bool} 是否成功
	 *  return true if successful
	 */
	bool cluster_delslots(int first, ...);
	bool cluster_delslots(const int slot_list[], size_t n);
	bool cluster_delslots(const std::vector<int>& slot_list);

	/**
	 * 获得某个哈希槽当前所存储对象的键名集合
	 * get keys array stored in one specified hash-slot
	 * @param slot {size_t} 哈希槽值
	 *  the specified hash-slot
	 * @param max {size_t} 限制的结果集数量
	 *  limit the max results count
	 * @param result {std::list<acl::string>&} 存储结果集
	 *  stored the results
	 * @return {int} 查询结果集的个数，-1 表示出错
	 *  >= 0 if OK, -1 if error
	 */
	int cluster_getkeysinslot(size_t slot, size_t max, std::list<string>& result);

	/**
	 * 在建立 redis 集群时，可以使用此命令让一个 redis 结点是连接别的结点
	 * let one redis node to link to the other redis node
	 * when buiding the redis cluster
	 * @param ip {const char*} 被连接的其它一个 redis 结点的 IP 地址
	 *  the other redis node's ip to be linked
	 * @param port {int} 被连接的其它一个 redis 结点的 port 端口
	 *  the other redis node's port to be linked
	 * @return {bool} 连接是否成功
	 *  if the linking is successful
	 */
	bool cluster_meet(const char* ip, int port);

	/**
	 * 重置一个 redis 结点的状态，使之从集群结点中脱离，清除哈希槽-结点的对应关系，
	 * 该方法等同于 reset_soft
	 * reset one redis node's status, escaping from the other nodes
	 * of the redis cluster, and clearing slot-to-nodes mapping;
	 * same as reset_soft
	 * @return {bool} 操作是否成功
	 *  if the operation is successful
	 */
	bool cluster_reset();
	bool cluster_reset_hard();
	bool cluster_reset_soft();

	/**
	 * 设定某个哈希槽在当前 redis 结点上正处于导入状态
	 * set the hash-slot in importing status from the other node
	 * to the current node
	 * @param slot {size_t} 哈希槽值
	 *  the hash-slot value
	 * @param src_node {const char*} 该哈希槽的 redis 源结点
	 *  the source redis-node of the hash-slot importing from
	 * @return {boo} 设置状态是否成功
	 *  if success for setting the slot's status
	 */
	bool cluster_setslot_importing(size_t slot, const char* src_node);

	/**
	 * 设定某个哈希槽在当前 redis 结点上正处于迁移状态
	 * set the hash-slot in migrating status to the other node
	 * from the current node
	 * @param slot {size_t} 哈希槽值
	 *  the hash-slot value
	 * @param dst_node {const char*} 该哈希槽的 redis 迁移目标结点
	 *  the target redis-node of the hash-slot migrating to
	 * @return {boo} 设置状态是否成功
	 *  if success for setting the slot's status
	 */
	bool cluster_setslot_migrating(size_t slot, const char* dst_node);

	/**
	 * 当导入/迁移哈希槽完成后使用此操作指定该哈希槽为稳定状态
	 * set the hash-slot stable after importing or migrating
	 * @param slot {size_t} 哈希槽值
	 *  the hash-slot value
	 * @return {bool} 设置状态是否成功
	 *  if success for setting the slot's status
	 */
	bool cluster_setslot_stable(size_t slot);

	/**
	 * 设置指定的哈希槽至指定的某个 redis 结点，该指令有较为复杂的行为特征，具体
	 * 请参见官方在线文档
	 * set one hash-slot to one redis node, for more help see online doc
	 * @param slot {size_t} 哈希槽值
	 *  the hash-slot to be set
	 * @param node {const char*} 接收该哈希槽的 redis 结点
	 *  the redis node to be holding the hash-slot
	 * @return {bool} 操作是否成功
	 *  if the operation is successful
	 */
	bool cluster_setslot_node(size_t slot, const char* node);

	/**
	 * 获得某个指定 redis 结点报错的数量
	 * get the count of the failure resports by one redis node
	 * @param node {const char*} 指定的某个 redis 结点
	 * @return {int} 结点报错的数量，正常情况下 >= 0，如果返回值 -1 表明操作出错
	 *  return the failure count reporting by the specified redis node,
	 *  return value >= 0 if successful, or -1 for error happened
	 */
	int cluster_count_failure_reports(const char* node);

	/**
	 * 该命令操作只能发送给一个从结点，用来对主结点进行故障转移，使当前的从结点
	 * 做为主结点，需要与它的主结点进行协商，同时需要获得其它主结点的认可
	 * this command can only be sent to one slave node for failover
	 * of the master node, make the current slave to be the master
	 * @return {bool} 操作是否成功
	 *  if the operation is successful
	 */
	bool cluster_failover();

	/**
	 * 强制性将一个从结点变为主结点，该操作不必与原来的主结点进行协商，但仍需得到
	 * 集群中大多数主结点的同意
	 * force a slave to be the master, not handshake with it's master,
	 * but still need get agreement by majority of the masters in cluster
	 * @return {bool} 操作是否成功
	 *  if the operation is successful
	 */
	bool cluster_failover_force();

	/**
	 * 强制性将一个从结点变为主结点，该操作不必与原来的主结点和集群中的其它主结点
	 * 进行协商
	 * force a slave to be the master, not handshake with it's master,
	 * and also no need get agreement by the other masters in cluster
	 * @return {bool} 操作是否成功
	 *  if the operation is successful
	 */
	bool cluster_failover_takeover();

	/**
	 * 获得当前集群的一些概述信息
	 * get running informantion about the redis cluster
	 * @param result {std::map<acl::string, acl::string>&} 存储结果
	 *  store the result of this operation
	 * @return {bool} 操作是否成功
	 *  if this operation is successful
	 */
	bool cluster_info(std::map<string, string>& result);

	/**
	 * 让当前 redis 结点将配置信息保存至磁盘的 nodes.conf 中
	 * let the current node to save the cluster information in nodes.conf
	 * @return {bool} 操作是否成功
	 *  if this operation is successful
	 */
	bool cluster_saveconfig();

	/**
	 * 获得某个哈希槽中的对象总数量
	 * get all the keys's count in one hash-slot
	 * @param slot {size_t} 指定哈希槽
	 *  the specified hash-slot
	 * @return {int} 返回哈希槽中的对象数量，-1 表示出错
	 *　return the keys's count in the hash-slot, return -1 if error 
	 */
	int cluster_countkeysinslot(size_t slot);

	/**
	 * 将指定结点从当前的结点中移除
	 * remove the specified node from the current node
	 * @param node {const char*} 指定的要被移除的结点
	 *  the speicied node to be removed
	 * @return {bool} 操作是否成功
	 *  if this operation is successful
	 */
	bool cluster_forget(const char* node);

	/**
	 * 获得某个键所属的哈希槽
	 * get the hash-slot wich the key belongs to
	 * @param key {const char*} 键值
	 *  the key string
	 * @return {int} 哈希槽值，>= 0 表示成功，-1 表示操作失败
	 *  return the key's hash-slot, >= 0 if successful, -1 on error
	 */
	int cluster_keyslot(const char* key);

	/**
	 * 将指定结点设置为从结点，如果该结点原来为从结点，则也会返回成功
	 * set the specified node to be a slave node
	 * @param node {const char*} 指定结点标识符
	 *  the specified node
	 * @return {bool} 操作是否成功
	 *  if this operation is successful
	 */
	bool cluster_replicate(const char* node);

	bool cluster_set_config_epoch(const char* epoch);

	/**
	 * 获得所有哈希槽在集群中各个 redis 结点的分布情况
	 * get all nodes with all slots
	 * @return {const std::vector<redis_slot*>*} 返回存储哈希槽信息
	 *  的所有主结点集合，返回 NULL 表示出错
	 *  return all the master nodes with all hash-slots in them,
	 *  and NULL will be returned if error happened, and the return
	 *  value needn't be freed because it can be freed internal
	 */
	const std::vector<redis_slot*>* cluster_slots();
	
	/**
	 * 获得当前集群中所有结点的主结点，主结点的所有从结点可以通过
	 * redis_node::get_slaves 获得
	 * get all the masters of the cluster, and master's slave nodes
	 * can be got by redis_node::get_slaves
	 * @return {const std::map<string, redis_node*>*} 返回 NULL 表示出错
	 *  return NULL if error happened, the return value needn't be
	 *  freed because it can be freed internal
	 */
	const std::map<string, redis_node*>* cluster_nodes();

	/**
	 * 获得指定主结点的所有从结点
	 * get all slave nodes of the specified master node
	 * @return node {const char*} 主结点标识符，返回 NULL 表示出错，该
	 *  返回结果不需要释放，内部自动维护
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
