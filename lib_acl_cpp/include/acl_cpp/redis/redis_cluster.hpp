#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include <list>
#include <map>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_command.hpp"

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
	redis_cluster();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_cluster(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*�� size_t)
	 */
	redis_cluster(redis_client_cluster* cluster, size_t max_conns);

	virtual ~redis_cluster();

	/**
	 * ������ӿ��õĹ�ϣ�ۣ���������С�� 0 �Ĺ�ϣ��ֵ��ʾ����
	 * add some hash-slots, the last slot value must be < 0 indicating
	 * the end of the slots array
	 * @param first {int} ��һ����ϣ�ۣ���ֵ���� >= 0 ����Ч
	 *  the first hash-slot which must be >= 0
	 * @param slot_list {const int[]} Ҫ��ӵĹ�ϣ�۵��б�
	 *  the hash-slots array to be added
	 * @param n {size_t} the count of the hash-slots list
	 * @param slot_list {const std::vector<init>&} Ҫ��ӵĹ�ϣ�۵��б�
	 *  the hash-slots list to be added
	 * @return {bool} �Ƿ�ɹ�
	 *  return true if successful
	 */
	bool cluster_addslots(int first, ...);
	bool cluster_addslots(const int slot_list[], size_t n);
	bool cluster_addslots(const std::vector<int>& slot_list);

	/**
	 * ����ɾ����ϣ�ۣ���������С�� 0 �Ĺ�ϣ�۱�ʾ����
	 * remove some hash-slots, the last slot value must be < 0 indicating
	 * the end of the slots array
	 * @param first {int} ��һ����ϣ�ۣ���ֵ���� >= 0 ����Ч
	 *  the first hash-slot which must be >= 0
	 * @param slot_list {const int[]} Ҫɾ���Ĺ�ϣ�۵��б�
	 *  the hash-slots array to be removed
	 * @param n {size_t} the count of the hash-slots list
	 * @param slot_list {const std::vector<init>&} Ҫɾ���Ĺ�ϣ�۵��б�
	 *  the hash-slots array to be removed
	 * @return {bool} �Ƿ�ɹ�
	 *  return true if successful
	 */
	bool cluster_delslots(int first, ...);
	bool cluster_delslots(const int slot_list[], size_t n);
	bool cluster_delslots(const std::vector<int>& slot_list);

	/**
	 * ���ĳ����ϣ�۵�ǰ���洢����ļ�������
	 * get keys array stored in one specified hash-slot
	 * @param slot {size_t} ��ϣ��ֵ
	 *  the specified hash-slot
	 * @param max {size_t} ���ƵĽ��������
	 *  limit the max results count
	 * @param result {std::list<acl::string>&} �洢�����
	 *  stored the results
	 * @return {int} ��ѯ������ĸ�����-1 ��ʾ����
	 *  >= 0 if OK, -1 if error
	 */
	int cluster_getkeysinslot(size_t slot, size_t max, std::list<string>& result);

	/**
	 * �ڽ��� redis ��Ⱥʱ������ʹ�ô�������һ�� redis ��������ӱ�Ľ��
	 * let one redis node to link to the other redis node
	 * when buiding the redis cluster
	 * @param ip {const char*} �����ӵ�����һ�� redis ���� IP ��ַ
	 *  the other redis node's ip to be linked
	 * @param port {int} �����ӵ�����һ�� redis ���� port �˿�
	 *  the other redis node's port to be linked
	 * @return {bool} �����Ƿ�ɹ�
	 *  if the linking is successful
	 */
	bool cluster_meet(const char* ip, int port);

	/**
	 * ����һ�� redis ����״̬��ʹ֮�Ӽ�Ⱥ��������룬�����ϣ��-���Ķ�Ӧ��ϵ��
	 * �÷�����ͬ�� reset_soft
	 * reset one redis node's status, escaping from the other nodes
	 * of the redis cluster, and clearing slot-to-nodes mapping;
	 * same as reset_soft
	 * @return {bool} �����Ƿ�ɹ�
	 *  if the operation is successful
	 */
	bool cluster_reset();
	bool cluster_reset_hard();
	bool cluster_reset_soft();

	/**
	 * �趨ĳ����ϣ���ڵ�ǰ redis ����������ڵ���״̬
	 * set the hash-slot in importing status from the other node
	 * to the current node
	 * @param slot {size_t} ��ϣ��ֵ
	 *  the hash-slot value
	 * @param src_node {const char*} �ù�ϣ�۵� redis Դ���
	 *  the source redis-node of the hash-slot importing from
	 * @return {boo} ����״̬�Ƿ�ɹ�
	 *  if success for setting the slot's status
	 */
	bool cluster_setslot_importing(size_t slot, const char* src_node);

	/**
	 * �趨ĳ����ϣ���ڵ�ǰ redis �����������Ǩ��״̬
	 * set the hash-slot in migrating status to the other node
	 * from the current node
	 * @param slot {size_t} ��ϣ��ֵ
	 *  the hash-slot value
	 * @param src_node {const char*} �ù�ϣ�۵� redis Ǩ��Ŀ����
	 *  the target redis-node of the hash-slot migrating to
	 * @return {boo} ����״̬�Ƿ�ɹ�
	 *  if success for setting the slot's status
	 */
	bool cluster_setslot_migrating(size_t slot, const char* dst_node);

	/**
	 * ������/Ǩ�ƹ�ϣ����ɺ�ʹ�ô˲���ָ���ù�ϣ��Ϊ�ȶ�״̬
	 * set the hash-slot stable after importing or migrating
	 * @param slot {size_t} ��ϣ��ֵ
	 *  the hash-slot value
	 * @return {bool} ����״̬�Ƿ�ɹ�
	 *  if success for setting the slot's status
	 */
	bool cluster_setslot_stable(size_t slot);

	/**
	 * ����ָ���Ĺ�ϣ����ָ����ĳ�� redis ��㣬��ָ���н�Ϊ���ӵ���Ϊ����������
	 * ��μ��ٷ������ĵ�
	 * set one hash-slot to one redis node, for more help see online doc
	 * @param slot {size_t} ��ϣ��ֵ
	 *  the hash-slot to be set
	 * @param node {const char*} ���ոù�ϣ�۵� redis ���
	 *  the redis node to be holding the hash-slot
	 * @return {bool} �����Ƿ�ɹ�
	 *  if the operation is successful
	 */
	bool cluster_setslot_node(size_t slot, const char* node);

	/**
	 * ���ĳ��ָ�� redis ��㱨�������
	 * get the count of the failure resports by one redis node
	 * @param node {const char*} ָ����ĳ�� redis ���
	 * @return {int} ��㱨������������������ >= 0���������ֵ -1 ������������
	 *  return the failure count reporting by the specified redis node,
	 *  return value >= 0 if successful, or -1 for error happened
	 */
	int cluster_count_failure_reports(const char* node);

	/**
	 * ���������ֻ�ܷ��͸�һ���ӽ�㣬�������������й���ת�ƣ�ʹ��ǰ�Ĵӽ��
	 * ��Ϊ����㣬��Ҫ��������������Э�̣�ͬʱ��Ҫ��������������Ͽ�
	 * this command can only be sent to one slave node for failover
	 * of the master node, make the current slave to be the master
	 * @return {bool} �����Ƿ�ɹ�
	 *  if the operation is successful
	 */
	bool cluster_failover();

	/**
	 * ǿ���Խ�һ���ӽ���Ϊ����㣬�ò���������ԭ������������Э�̣�������õ�
	 * ��Ⱥ�д����������ͬ��
	 * force a slave to be the master, not handshake with it's master,
	 * but still need get agreement by majority of the masters in cluster
	 * @return {bool} �����Ƿ�ɹ�
	 *  if the operation is successful
	 */
	bool cluster_failover_force();

	/**
	 * ǿ���Խ�һ���ӽ���Ϊ����㣬�ò���������ԭ���������ͼ�Ⱥ�е����������
	 * ����Э��
	 * force a slave to be the master, not handshake with it's master,
	 * and also no need get agreement by the other masters in cluster
	 * @return {bool} �����Ƿ�ɹ�
	 *  if the operation is successful
	 */
	bool cluster_failover_takeover();

	/**
	 * ��õ�ǰ��Ⱥ��һЩ������Ϣ
	 * get running informantion about the redis cluster
	 * @param result {std::map<acl::string, acl::string>&} �洢���
	 *  store the result of this operation
	 * @return {bool} �����Ƿ�ɹ�
	 *  if this operation is successful
	 */
	bool cluster_info(std::map<string, string>& result);

	/**
	 * �õ�ǰ redis ��㽫������Ϣ���������̵� nodes.conf ��
	 * let the current node to save the cluster information in nodes.conf
	 * @return {bool} �����Ƿ�ɹ�
	 *  if this operation is successful
	 */
	bool cluster_saveconfig();

	/**
	 * ���ĳ����ϣ���еĶ���������
	 * get all the keys's count in one hash-slot
	 * @param slot {size_t} ָ����ϣ��
	 *  the specified hash-slot
	 * @return {int} ���ع�ϣ���еĶ���������-1 ��ʾ����
	 *��return the keys's count in the hash-slot, return -1 if error 
	 */
	int cluster_countkeysinslot(size_t slot);

	/**
	 * ��ָ�����ӵ�ǰ�Ľ�����Ƴ�
	 * remove the specified node from the current node
	 * @param node {const char*} ָ����Ҫ���Ƴ��Ľ��
	 *  the speicied node to be removed
	 * @return {bool} �����Ƿ�ɹ�
	 *  if this operation is successful
	 */
	bool cluster_forget(const char* node);

	/**
	 * ���ĳ���������Ĺ�ϣ��
	 * get the hash-slot wich the key belongs to
	 * @param key {const char*} ��ֵ
	 *  the key string
	 * @return {int} ��ϣ��ֵ��>= 0 ��ʾ�ɹ���-1 ��ʾ����ʧ��
	 *  return the key's hash-slot, >= 0 if successful, -1 on error
	 */
	int cluster_keyslot(const char* key);

	/**
	 * ��ָ���������Ϊ�ӽ�㣬����ý��ԭ��Ϊ�ӽ�㣬��Ҳ�᷵�سɹ�
	 * set the specified node to be a slave node
	 * @param node {const char*} ָ������ʶ��
	 *  the specified node
	 * @return {bool} �����Ƿ�ɹ�
	 *  if this operation is successful
	 */
	bool cluster_replicate(const char* node);

	bool cluster_set_config_epoch(const char* epoch);

	/**
	 * ������й�ϣ���ڼ�Ⱥ�и��� redis ���ķֲ����
	 * get all nodes with all slots
	 * @return {const std::vector<redis_slot*>*} ���ش洢��ϣ����Ϣ
	 *  ����������㼯�ϣ����� NULL ��ʾ����
	 *  return all the master nodes with all hash-slots in them,
	 *  and NULL will be returned if error happened, and the return
	 *  value needn't be freed because it can be freed internal
	 */
	const std::vector<redis_slot*>* cluster_slots();
	
	/**
	 * ��õ�ǰ��Ⱥ�����н�������㣬���������дӽ�����ͨ��
	 * redis_node::get_slaves ���
	 * get all the masters of the cluster, and master's slave nodes
	 * can be got by redis_node::get_slaves
	 * @return {const std::map<string, redis_node*>*} ���� NULL ��ʾ����
	 *  return NULL if error happened, the return value needn't be
	 *  freed because it can be freed internal
	 */
	const std::map<string, redis_node*>* cluster_nodes();

	/**
	 * ���ָ�����������дӽ��
	 * get all slave nodes of the specified master node
	 * @return node {const char*} ������ʶ�������� NULL ��ʾ������
	 *  ���ؽ������Ҫ�ͷţ��ڲ��Զ�ά��
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
