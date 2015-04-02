#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include <list>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_node.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class redis_result;

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
	 * see redis_command::redis_command(redis_client_cluster*， size_t)
	 */
	redis_cluster(redis_client_cluster* cluster, size_t max_conns);

	virtual ~redis_cluster();

	/**
	 * 批量添加可用的哈希槽，最后必须以小于 0 的哈希槽值表示结束
	 * add some hash-slots, the last slot value must be < 0
	 * @param first {int} 第一个哈希槽，该值必须 >= 0 才有效
	 *  the first hash-slot which must be >= 0
	 * @return {bool} 是否成功
	 *  return true if success
	 */
	bool addslots(int first, ...);
	bool addslots(const int slot_list[], size_t n);
	bool addslots(const std::vector<int>& slot_list);

	bool delslots(int first, ...);
	bool delslots(const int slot_list[], size_t n);
	bool delslots(const std::vector<int>& slot_list);

	int getkeysinslot(size_t slot, size_t max, std::list<string>& result);

	bool meet(const char* ip, int port);
	bool reset();
	bool reset_hard();
	bool reset_soft();

	bool setslot_importing(size_t slot, const char* src_node);
	bool setslot_migrating(size_t slot, const char* dst_node);
	bool setslot_stable(size_t slot);
	bool setslot_node(size_t slot, const char* node);

	int count_failure_reports(const char* node);

	bool failover();
	bool failover_force();
	bool failover_takeover();

	bool info(string& result);
	bool nodes(string& result);
	bool saveconfig();
	bool slaves(const char* node, std::vector<string>& result);
	int countkeysinslot(size_t slot);
	bool forget(const char* node);
	int keyslot(const char* key);
	bool replicate(const char* node);
	bool set_config_epoch(const char* epoch);
	const std::vector<redis_node*>* slots();

private:
	std::vector<redis_node*> nodes_;

	redis_node* get_master_node(const redis_result* rr);
	redis_node* create_node(const redis_result* rr,
		size_t slot_max, size_t slot_min);
	void free_nodes();
};

} // namespace acl
