#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <map>
#include "acl_cpp/redis/redis_result.hpp"

namespace acl
{

class redis_client;
class redis_pool;
class redis_cluster;
class redis_request;

/**
 * redis 客户端命令类的纯虚父类;
 * the redis command classes's base virtual class, which includes the basic
 * functions for all sub-classes
 */
class ACL_CPP_API redis_command
{
public:
	redis_command();
	redis_command(redis_client* conn);
	redis_command(redis_cluster* cluster, size_t max_conns);
	virtual ~redis_command() = 0;

	/**
	 * 在重复使用一个继承于 redis_command 的子类操作 redis 时，需要在
	 * 下一次调用前调用本方法以释放上次操作的临时对象;
	 * when reusing a redis command sub-class, the reset method should be
	 * called first to rlease some resources in last command operation
	 * @param save_slot {bool} 当采用集群模式时，该参数决定是否需要重新
	 *  计算哈希槽值，如果反复调用 redis 命令过程中的 key 值不变，则可以保留此
	 *  哈希槽值以减少内部重新进行计算的次数;
	 *  when in cluster mode, if your operations is on the same key, you
	 *  can set the param save_slot to false which can reduse the times
	 *  of compute the same key's hash-slot.
	 */
	void reset(bool save_slot = false);

	/**
	 * 在使用连接池方式时，通过本函数将从连接池获得的连接对象;
	 * when using connection pool and not in cluster mode, the function
	 * is used to set the connection for next redis command operation.
	 * @param conn {redis_client*} 与 redis 客户端命令进行关联;
	 *  the redis connection to be set in next redis operation
	 */
	void set_client(redis_client* conn);

	/**
	 * 获得当前 redis 客户端命令的连接对象;
	 * get redis connection set by set_client function
	 * @return {redis_client*} 返回 NULL 表示没有连接对象与当前的命令对象
	 *  进行绑定;
	 *  the internal redis connection be returned, NULL if no redis
	 *  connection be set 
	 */
	redis_client* get_client() const
	{
		return conn_;
	}

	/**
	 * 设置连接池集群管理器;
	 * set the redis cluster object in redis cluster mode
	 * @param cluster {redis_cluster*} redis 集群连接对象;
	 *  the redis_cluster connection object which can connect to any
	 *  redis-server and support connection pool
	 * @param max_conns {size_t} 当内部动态创建连接池对象时，该值指定每个动态创建
	 *  的连接池的最大连接数量;
	 *  when dynamicly creating connection pool to any redis-server, use
	 *  this param to limit the max number for each connection pool
	 */
	void set_cluster(redis_cluster* cluster, size_t max_conns);

	/**
	 * 获得所设置的连接池集群管理器;
	 * get redis_cluster object set by set_cluster function
	 * @return {redis_cluster*}
	 */
	redis_cluster* get_cluster() const
	{
		return cluster_;
	}

	/**
	 * 获得内存池句柄，该内存池由 redis_command 内部产生;
	 * get memory pool handle be set
	 * @return {dbuf_pool*}
	 */
	dbuf_pool* get_pool() const
	{
		return pool_;
	}

	/**
	 * 获得当前结果结点的数据类型;
	 * get the result type returned from redis-server
	 * @return {redis_result_t}
	 */
	redis_result_t result_type() const;

	/**
	 * 当返回值为 REDIS_RESULT_STATUS 类型时，本方法返回状态信息;
	 * when result type is REDIS_RESULT_STATUS, the status info can be
	 * get by this function
	 * @return {const char*} 返回 "" 表示出错;
	 *  "" will be returned on error
	 */
	const char* result_status() const;

	/**
	 * 当出错时返回值为 REDIS_RESULT_ERROR 类型，本方法返回出错信息;
	 * when result type is REDIS_RESULT_ERROR, the error info can be
	 * get by this function
	 * @return {const char*} 返回空串 "" 表示没有出错信息;
	 *  "" will be returned when no error info
	 */
	const char* result_error() const;

	/**
	 * 获得当前结果结点存储的对象的个数, 该方法可以获得结果为下面两个方法
	 * (get_child/get_value) 所需要的数组元素的个数;
	 * get number of result objects, just for functions get_child/get_value 
	 * @return {size_t} 返回值与存储类型的对应关系如下：
	 *  the relation between return value and result type, as below:
	 *  REDIS_RESULT_ERROR: 1
	 *  REDIS_RESULT_STATUS: 1
	 *  REDIS_RESULT_INTEGER: 1
	 *  REDIS_RESULT_STRING: > 0 时表示该字符串数据被切分成非连接内存块的个数;
	 *       when the result type is REDIS_RESULT_STRING and the the
	 *       string is too large, the string was be cut into many small
	 *       chunks, the returned value is the chunks number
	 *  REDIS_RESULT_ARRAY: children_->size()
	 */
	size_t result_size() const;

	/**
	 * 当返回值为 REDIS_RESULT_INTEGER 类型时，本方法返回对应的 32 位整数值;
	 * get 32-bits number value if result type is REDIS_RESULT_INTERGER
	 * @param success {bool*} 本指针非 NULL 时记录操作过程是否成功;
	 *  if the param pointer is not NULL, which will save status of
	 *  success or not for result from redis-server
	 * @return {int}
	 */
	int result_number(bool* success = NULL) const;

	/**
	 * 当返回值为 REDIS_RESULT_INTEGER 类型时，本方法返回对应的 64 位整数值;
	 * get 64-bits number value if result type is REDIS_RESULT_INTERGER
	 * @param success {bool*} 本指针非 NULL 时记录操作过程是否成功;
	 *  if the param pointer is not NULL, which will save status of
	 *  success or not for result from redis-server
	 * @return {long long int}
	 */
	long long int result_number64(bool* success = NULL) const;

	/**
	 * 返回对应下标的数据(当数据类型非 REDIS_RESULT_ARRAY 时）;
	 * get string result when result type isn't REDIS_RESULT_ARRAY
	 * @param i {size_t} 数组下标;
	 *  the array index
	 * @param len {size_t*} 当为非 NULL 指针时存储所返回数据的长度;
	 *  *len will save the result's length if len is not NULL
	 * @return {const char*} 返回 NULL 表示下标越界;
	 *  NULL will return if i beyonds the array's size
	 */
	const char* get_result(size_t i, size_t* len = NULL) const;

	/**
	 * 判断当前所绑定的 redis 连接流对象(redis_client) 连接是否已经关闭；
	 * 只有内部的 conn_ 流对象非空时调用此函数才有意义;
	 * to judge if the redis connection was be closed, only redis_client
	 * object be set internal
	 * @return {bool}
	 */
	bool eof() const;

	/**
	 * 获得本次 redis 操作过程的结果;
	 * get result object of last redis operation
	 * @return {redis_result*}
	 */
	const redis_result* get_result() const;

	/**
	 * 当查询结果为数组对象时调用本方法获得一个数组元素对象;
	 * get one result ojbect of array if result type is REDIS_RESULT_ARRAY
	 * @param i {size_t} 数组对象的下标值;
	 *  the result array's index
	 * @return {const redis_result*} 当结果非数组对象或结果为空或出错时
	 *  该方法返回 NULL;
	 *  NULL will be resturned when result is not REDIS_RESULT_ARRAY or
	 *  array empty or error
	 */
	const redis_result* result_child(size_t i) const;

	/**
	 * 当从 redis-server 获得的数据是一组字符串类型的结果集时，可以调用
	 * 本函数获得某个指定下标位置的数据;
	 * @param i {size_t} 下标（从 0 开始）
	 * @param len {size_t*} 若该指针非空，则存储所返回结果的长度（仅当该
	 *  方法返回非空指针时有效）
	 * @return {const char*} 返回对应下标的值，当返回 NULL 时表示该下标没
	 *  有值，为了保证使用上的安全性，返回的数据总能保证最后是以 \0 结尾，
	 *  在计算数据长度时不包含该结尾符，但为了兼容二进制情形，调用者还是
	 *  应该通过返回的 len 存放的长度值来获得数据的真实长度
	 */
	const char* result_value(size_t i, size_t* len = NULL) const;

	/////////////////////////////////////////////////////////////////////
	/**
	 * 设置是否对请求数据进行分片处理，如果为 true 则内部在组装请求协议的时候不会
	 * 将所有数据块重新组装成一个连续的大数据块
	 * @param on {bool} 内部默认值为 false
	 */
	void set_slice_request(bool on);

	/**
	 * 设置是否对响应数据进行分片处理，如果为 true 则当服务器的返回数据比较大时则
	 * 将数据进行分片，分成一些不连续的数据块
	 * @param on {bool} 内部默认值为 false
	 */
	void set_slice_respond(bool on);

protected:
	const redis_result* run(size_t nchild = 0);
	const redis_result* run(redis_cluster* cluster, size_t nchild);

	void build_request(size_t argc, const char* argv[], size_t lens[]);
	void reset_request();
	const redis_result** scan_keys(const char* cmd, const char* key,
		int& cursor, size_t& size, const char* pattern,
		const size_t* count);
	/*******************************************************************/

	void build(const char* cmd, const char* key,
		const std::map<string, string>& attrs);
	void build(const char* cmd, const char* key,
		const std::map<string, const char*>& attrs);

	void build(const char* cmd, const char* key,
		const std::vector<string>& names,
		const std::vector<string>& values);
	void build(const char* cmd, const char* key,
		const std::vector<const char*>& names,
		const std::vector<const char*>& values);

	void build(const char* cmd, const char* key,
		const char* names[], const char* values[], size_t argc);
	void build(const char* cmd, const char* key,
		const int names[], const char* values[], size_t argc);
	void build(const char* cmd, const char* key,
		const char* names[], const size_t names_len[],
		const char* values[], const size_t values_len[], size_t argc);

	/*******************************************************************/

	void build(const char* cmd, const char* key,
		const std::vector<string>& names);
	void build(const char* cmd, const char* key,
		const std::vector<const char*>& names);

	void build(const char* cmd, const char* key,
		const char* names[], size_t argc);
	void build(const char* cmd, const char* key,
		const char* names[], const size_t lens[], size_t argc);

protected:
	int get_number(bool* success = NULL);
	long long int get_number64(bool* success = NULL);
	int get_number(std::vector<int>& out);
	int get_number64(std::vector<long long int>& out);
	bool check_status(const char* success = "OK");

	int get_status(std::vector<bool>& out);
	const char* get_status();

	int get_string(string& buf);
	int get_string(string* buf);
	int get_string(char* buf, size_t size);
	int get_strings(std::vector<string>& result);
	int get_strings(std::vector<string>* result);
	int get_strings(std::map<string, string>& result);
	int get_strings(std::vector<string>& names,
		std::vector<string>& values);
	int get_strings(std::vector<const char*>& names,
		std::vector<const char*>& values);

	/************************** common *********************************/
protected:
	dbuf_pool* pool_;

	// 根据键值计算哈希槽值
	void hash_slot(const char* key);
	void hash_slot(const char* key, size_t len);

private:
	redis_client* conn_;
	redis_cluster* cluster_;
	size_t max_conns_;
	unsigned long long used_;
	int slot_;
	int redirect_max_;
	int redirect_sleep_;

	redis_client* peek_conn(redis_cluster* cluster, int slot);
	redis_client* redirect(redis_cluster* cluster, const char* addr);
	const char* get_addr(const char* info);

private:
	/************************** request ********************************/
	bool slice_req_;
	string* request_buf_;
	redis_request* request_obj_;
	size_t  argv_size_;
	const char**  argv_;
	size_t* argv_lens_;
	size_t  argc_;

	void argv_space(size_t n);
	void build_request1(size_t argc, const char* argv[], size_t lens[]);
	void build_request2(size_t argc, const char* argv[], size_t lens[]);

private:
	/************************** respond ********************************/
	bool slice_res_;
	const redis_result* result_;
};

} // namespace acl
