#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <map>
#include <list>
#include <vector>
#include "acl_cpp/redis/redis_result.hpp"

namespace acl
{

class redis_client;
class redis_client_cluster;

/**
 * redis 客户端命令类的纯虚父类;
 * the redis command classes's base virtual class, which includes the basic
 * functions for all sub-classes
 */
class ACL_CPP_API redis_proxy
{
public:

	redis_proxy(redis_client_cluster* cluster, size_t max_conns);
    ~redis_proxy();
	void clear(bool save_slot = false);

	ACL_CPP_DEPRECATED_FOR("clear")
	void reset(bool save_slot = false);

	void set_client(redis_client* conn);

	redis_client* get_client() const
	{
		return conn_;
	}

	/**
	 * 获得当前 redis 命令对象所绑定的服务器地址，只有当该对象与 redis_client
	 * 绑定时（即调用 set_client) 才可以调用本函数
	 * get the redis-server's addr used by the current command. this
	 * method can only be used only if the redis_client was set by
	 * set_client method.
	 * @return {const char*} 返回空串 "" 表示没有绑定 redis 连接对象
	 *  if "" was resturned, the redis connection was not set
	 */
	const char* get_client_addr() const;

	/**
	 * 设置连接池集群管理器;
	 * set the redis cluster object in redis cluster mode
	 * @param cluster {redis_client_cluster*} redis 集群连接对象;
	 *  the redis_cluster connection object which can connect to any
	 *  redis-server and support connection pool
	 * @param max_conns {size_t} 当内部动态创建连接池对象时，该值指定每个动态创建
	 *  的连接池的最大连接数量;
	 *  when dynamicly creating connection pool to any redis-server, use
	 *  this param to limit the max number for each connection pool
	 */
	void set_cluster(redis_client_cluster* cluster, size_t max_conns);

	/**
	 * 获得所设置的连接池集群管理器;
	 * get redis_cluster object set by set_cluster function
	 * @return {redis_client_cluster*}
	 */
	redis_client_cluster* get_cluster() const
	{
		return cluster_;
	}

	/**
	 * 获得内存池句柄，该内存池由 redis_command 内部产生;
	 * get memory pool handle be set
	 * @return {dbuf_pool*}
	 */
	dbuf_pool* get_dbuf() const
	{
		return dbuf_;
	}


	bool eof() const;

	/**
	 * 获得本次 redis 操作过程的结果;
	 * get result object of last redis operation
	 * @return {redis_result*}
	 */
	const redis_result* get_result() const;

	// 根据键值计算哈希槽值
	void hash_slot(const char* key);
	void hash_slot(const char* key, size_t len);
	const redis_result* run(size_t nchild = 0);
	const redis_result* run(redis_client_cluster* cluster, size_t nchild);
	bool build_request(const char* data, size_t len);
	void clear_request();

	/************************** common *********************************/
protected:
	dbuf_pool* dbuf_;

private:
	char  addr_[32];
	redis_client* conn_;
	redis_client_cluster* cluster_;
	size_t max_conns_;
	unsigned long long used_;
	int slot_;
	int redirect_max_;
	int redirect_sleep_;
	string* request_buf_;

	redis_client* peek_conn(redis_client_cluster* cluster, int slot);
	redis_client* redirect(redis_client_cluster* cluster, const char* addr);
	const char* get_addr(const char* info);
	void set_client_addr(const char* addr);
	void set_client_addr(redis_client& conn);

private:
	const redis_result* result_;

	void logger_result(const redis_result* result);
};

} // namespace acl
