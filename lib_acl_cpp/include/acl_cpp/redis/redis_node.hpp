#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include <utility>
#include "acl_cpp/stdlib/string.hpp"

namespace acl
{

/**
 * 该类主要用于 redis_cluster 命令类获取有关集群 redis 结点信息
 * this class is mainly used for redis_cluster command class to
 * get some information about the nodes in redis cluster
 */
class ACL_CPP_API redis_node
{
public:
	/**
	 * 当使用此构造函数实例化类对象时，需要调用 set_id 和 set_addr 方法设置
	 * 该 redis 结点的唯一标识符及服务监听地址，同时还可调用其它的 set_xxx 设置方法
	 */
	redis_node(void);
	~redis_node(void);

	/**
	 * 除了在构造函数中的参数中传入该结点的 ID 标识符外，还可以通过此函数设置
	 * set the node's  ID
	 * @param id {const char*} 集群中 redis 结点的唯一标识符
	 *  the unique ID for one redis node in the reids cluster
	 * @return {redis_node&}
	 */
	redis_node& set_id(const char* id);

	/**
	 * 除了在构造函数中的参数中传入该结点的地址外，还可以通过此函数设置
	 * set the node's listening addr
	 * @param addr {const char*} 集群中 redis 结点的服务地址，格式：ip:port
	 *  the listening addr of one redis node in the reids cluster
	 * @return {redis_node&}
	 */
	redis_node& set_addr(const char* addr);

	/**
	 * 设置当前结点的类型
	 * set the current node's type
	 * @param type {const char*}
	 * @return {redis_node&}
	 */
	redis_node& set_type(const char* type);

	/**
	 * 设置当前结点是否为当前的连接对象
	 * set if the current node is belonging to the current connection
	 * @param yesno {bool}
	 * @return {redis_node&}
	 */
	redis_node& set_myself(bool yesno);

	/**
	 * 当本结点为从结点时，设置当前结点的主结点
	 * setting current slave node's master node
	 * @param master {const redis_node*} 主结点对象
	 *  the redis master node of the current slave in cluster
	 * @return {redis_node&}
	 */
	redis_node& set_master(const redis_node* master);

	/**
	 * 设置当前结点正处于握手阶段
	 * set the current node being in handshaking status
	 * @param yesno {bool}
	 * @return {redis_node&}
	 */
	redis_node& set_handshaking(bool yesno);

	/**
	 * 设置当前结点处于连线状态
	 * set the node been connected in the cluster
	 * @param yesno {bool}
	 * @return {redis_node&}
	 */
	redis_node& set_connected(bool yesno);

	/**
	 * 当本结点为从结点时，设置当前结点的主结点标识符
	 * setting current node's master node when the node is slave node
	 * @param id {const char*} 主结点唯一标识符
	 *  the unique ID of the master node
	 * @return {redis_node&}
	 */
	redis_node& set_master_id(const char* id);

	/**
	 * 当本结点为主结点时，添加一个从结点
	 * add one slave node to the current node if it's one master node
	 * @return {bool} 添加是否成功，当从结点已经存在于当前主结点时则返回 false
	 *  false will be returned when the slave to be added is already
	 *  existing in the current master node
	 */
	bool add_slave(redis_node* slave);

	/**
	 * 当本结点为主结点时，根据结点唯一标识符删除一个从结点
	 * when the current node is a master node, this function will
	 * remove one slave node by the unique ID
	 * @param id {const char*} redis 结点唯一标识符
	 *  the unique ID of the redis node
	 * @return {const redis_node*} 返回被删除的从结点，如果不存在则返回 NULL
	 *  the slave node according to the ID will be returned, and if
	 *  not exists NULL will be returned
	 */
	redis_node* remove_slave(const char* id);

	/**
	 * 当本结点为主结点时，清空本结点的所有从结点
	 * clear all the slave nodes in the current master node
	 * @param free_all {bool} 是否需要同时释放这些从结点
	 *  if freeing the all slave nodes memory meanwhile
	 */
	void clear_slaves(bool free_all = false);

	/**
	 * 当本结点为主结点时，添加哈希槽范围
	 * add hash-slots range to the master node
	 * @param min {size_t} 哈希槽范围的起始值
	 *  the begin hash-slot of the slots range
	 * @param max {size_t} 哈希槽范围的结束值
	 *  the end hash-slot of the slots range
	 */
	void add_slot_range(size_t min, size_t max);

	/**
	 * 当本结点为主结点时，则获得主结点的哈希槽范围，当为从结点时则获得其对应的
	 * 主结点的哈希槽范围
	 * @return {const std::vector<std::pair<size_t, size_t> >&}
	 */
	const std::vector<std::pair<size_t, size_t> >& get_slots() const;

	/**
	 * 获得当前结点的类型
	 * get the node's type
	 * @return {const char*}
	 */
	const char* get_type() const
	{
		return type_.c_str();
	}

	/**
	 * 判断当前结点是否为当前的连接对象结点
	 * check if the node belongs to the current connection
	 * @return {bool}
	 */
	bool is_myself() const
	{
		return myself_;
	}

	/**
	 * 判断当前结点是否正处于握手阶段
	 * check if the node is in handshaking status
	 * @return {bool}
	 */
	bool is_handshaking() const
	{
		return handshaking_;
	}

	/**
	 * 判断当前结点是否已经处于连线状态
	 * check if the node is connected in the cluster
	 * @return {bool}
	 */
	bool is_connected() const
	{
		return connected_;
	}

	/**
	 * 当本结点为从结点时，获得该从结点的主结点对象
	 * get the current slave's master node
	 * @return {const redis_node*}
	 */
	const redis_node* get_master() const
	{
		return master_;
	}

	/**
	 * 当本结点为从结点时，获得该从结点对应的主结点的 ID 标识
	 * when the current node is slave, getting its master's ID
	 * @return {const char*}
	 */
	const char* get_master_id() const
	{
		return master_id_.c_str();
	}

	/**
	 * 当本结点为主结点时，获得该主结点的所有从结点
	 * getting all the slaves of the master
	 * @return {const std::vector<redis_node*>*}
	 */
	const std::vector<redis_node*>* get_slaves() const
	{
		return &slaves_;
	}

	/**
	 * 判断当前结点是否为集群中的一个主结点
	 * check if the current node is a master in the redis cluster
	 * @return {bool}
	 */
	bool is_master() const
	{
		return master_ == this;
	}

	/**
	 * 获得当前结点的 ID 标识
	 * get the unique ID of the current node, set in constructor
	 * @return {const char*}
	 */
	const char* get_id() const
	{
		return id_.c_str();
	}

	/**
	 * 获得当前结点的监听地址
	 * get the listening addr of the current node, set in constructor
	 * @reutrn {const char*}
	 */
	const char* get_addr() const
	{
		return addr_.c_str();
	}

private:
	string id_;
	string addr_;
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
