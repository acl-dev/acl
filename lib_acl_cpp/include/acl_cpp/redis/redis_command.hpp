#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include "../stdlib/tbox.hpp"
#include <map>
#include <list>
#include <vector>
#include "redis_result.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

class redis_request;
class redis_client;
class redis_client_cluster;
class redis_client_pipeline;
class redis_pipeline_message;

/**
 * redis 客户端命令类的纯虚父类;
 * the redis command classes's base virtual class, which includes the basic
 * functions for all sub-classes
 */
class ACL_CPP_API redis_command : public noncopyable
{
public:
	/**
	 * 缺省的构造函数，如果使用此构造函数初始化类对象，则必须调用
	 * set_client 或 set_cluster 设置 redis 客户端命令类对象的通讯方式。
	 * default constructor. You must set the communication method by
	 * set_client or set_cluster functions.
	 */
	redis_command(void);

	/**
	 * 当使用非集群模式时，可以使用此构造函数设置 redis 通信对象。
	 * Using this constructor to set the redis communication mode,
	 * usually in no-cluster mode.
	 * @param conn {redis_client*} redis 通信类对象
	 *  the redis communication in no-cluster mode
	 */
	redis_command(redis_client* conn);

	/**
	 * 集群模式的构造函数，在构造类对象时指定了集群模式的
	 * redis_client_cluster 对象。
	 * Using this constructor to set the redis_client_cluster, usually in
	 * cluster mode.
	 * @param cluster {redis_client_cluster*} redis 集群连接对象
	 *  redis cluster object in cluster mode
	 *  the max of every connection pool with all the redis nodes,
	 *  if be set 0, then there is no connections limit in
	 *  connections pool.
	 */
	redis_command(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_command(redis_client_cluster* cluster, size_t max_conns);

	redis_command(redis_client_pipeline* pipeline);

	virtual ~redis_command(void);

	/**
	 * 在进行每个命令处理前，是否要求检查 socket 句柄与地址的匹配情况，当
	 * 打开该选项时，将会严格检查匹配情况，但会影响一定性能，因此该设置仅
	 * 用在 DEBUG 时的运行场景
	 * @param on {bool}
	 */
	void set_check_addr(bool on);

	/**
	 * 在重复使用一个继承于 redis_command 的子类操作 redis 时，需要在
	 * 下一次调用前调用本方法以释放上次操作的临时对象;
	 * when reusing a redis command sub-class, the reset method should be
	 * called first to rlease some resources in last command operation
	 * @param save_slot {bool} 当采用集群模式时，该参数决定是否需要重新
	 *  计算哈希槽值，如果反复调用 redis 命令过程中的 key 值不变，则可以保
	 *  留此哈希槽值以减少内部重新进行计算的次数;
	 *  when in cluster mode, if your operations is on the same key, you
	 *  can set the param save_slot to false which can reduse the times
	 *  of compute the same key's hash-slot.
	 */
	void clear(bool save_slot = false);

	ACL_CPP_DEPRECATED_FOR("clear")
	void reset(bool save_slot = false);

	/**
	 * 在使用非集群方式时，通过本函数将从连接池获得的连接对象;
	 * when not using cluster mode, the function is used
	 * to set the connection for next redis command operation.
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
	redis_client* get_client(void) const
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
	const char* get_client_addr(void) const;

	/**
	 * 设置连接池集群管理器;
	 * set the redis cluster object in redis cluster mode
	 * @param cluster {redis_client_cluster*} redis 集群连接对象;
	 *  the redis_cluster connection object which can connect to any
	 *  redis-server and support connection pool
	 *  when dynamicly creating connection pool to any redis-server, use
	 *  this param to limit the max number for each connection pool
	 */
	void set_cluster(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	void set_cluster(redis_client_cluster* cluster, size_t max_conns);

	/**
	 * 获得所设置的连接池集群管理器;
	 * get redis_cluster object set by set_cluster function
	 * @return {redis_client_cluster*}
	 */
	redis_client_cluster* get_cluster(void) const
	{
		return cluster_;
	}

	void set_pipeline(redis_client_pipeline* pipeline);
	redis_client_pipeline* get_pipeline(void) const
	{
		return pipeline_;
	}

	/**
	 * 获得内存池句柄，该内存池由 redis_command 内部产生;
	 * get memory pool handle been set
	 * @return {dbuf_pool*}
	 */
	dbuf_pool* get_dbuf(void) const
	{
		return dbuf_;
	}

	/**
	 * 获得当前结果结点的数据类型;
	 * get the result type returned from redis-server
	 * @return {redis_result_t}
	 */
	redis_result_t result_type(void) const;

	/**
	 * 当返回值为 REDIS_RESULT_STATUS 类型时，本方法返回状态信息;
	 * when result type is REDIS_RESULT_STATUS, the status info can be
	 * get by this function
	 * @return {const char*} 返回 "" 表示出错;
	 *  "" will be returned on error
	 */
	const char* result_status(void) const;

	/**
	 * 当出错时返回值为 REDIS_RESULT_ERROR 类型，本方法返回出错信息;
	 * when result type is REDIS_RESULT_ERROR, the error info can be
	 * get by this function
	 * @return {const char*} 返回空串 "" 表示没有出错信息;
	 *  "" will be returned when no error info
	 */
	const char* result_error(void) const;

	/**
	 * 获得当前结果结点存储的对象的个数, 该方法可以获得结果为下面两个方法
	 * (result_child/result_value) 所需要的数组元素的个数;
	 * get number of result objects, just for functions
	 * result_child/result_value 
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
	size_t result_size(void) const;

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
	bool eof(void) const;

	/**
	 * 获得本次 redis 操作过程的结果;
	 * get result object of last redis operation
	 * @return {redis_result*}
	 */
	const redis_result* get_result(void) const;

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
	 * when the reply from redis-serveer are strings array, this
	 * function can be used to get the string specified by a subscript
	 * @param i {size_t} 下标（从 0 开始）
	 *  the subscript of strings array
	 * @param len {size_t*} 若该指针非空，则存储所返回结果的长度（仅当该
	 *  方法返回非空指针时有效）
	 *  if len not a NULL pointer, it will store the length of string
	 *  specified by the subscript
	 * @return {const char*} 返回对应下标的值，当返回 NULL 时表示该下标没
	 *  有值，为了保证使用上的安全性，返回的数据总能保证最后是以 \0 结尾，
	 *  在计算数据长度时不包含该结尾符，但为了兼容二进制情形，调用者还是
	 *  应该通过返回的 len 存放的长度值来获得数据的真实长度
	 *  the string will be returned associate with the subscript, if there
	 *  are nothing with the subscript, NULL will be returned
	 */
	const char* result_value(size_t i, size_t* len = NULL) const;

	/////////////////////////////////////////////////////////////////////
	/**
	 * 设置是否对请求数据进行分片处理，如果为 true 则内部在组装请求协议的
	 * 时候不会将所有数据块重新组装成一个连续的大数据块
	 * just for request package, setting flag for sending data with
	 * multi data chunks; this is useful when the request data is large
	 * @param on {bool} 内部默认值为 false
	 *  if true the request data will not be combined one package,
	 *  internal default is false
	 */
	void set_slice_request(bool on);

	/**
	 * 设置是否对响应数据进行分片处理，如果为 true 则当服务器的返回数据
	 * 比较大时则将数据进行分片，分成一些不连续的数据块
	 * just for response package, settint flag for receiving data
	 * if split the large response data into multi little chunks
	 * @param on {bool} 内部默认值为 false
	 *  if true the response data will be splitted into multi little
	 *  data, which is useful for large reponse data for avoiding
	 *  malloc large continuously memory from system.
	 *  internal default is false
	 */
	void set_slice_respond(bool on);

public:
	/**
	 * 直接组合 redis 协议命令方式，从 redis 服务器获得结果
	 * @param argc {size_t} 后面数组中数组元素个数
	 * @param argv {const char*[]} redis 命令组成的数组
	 * @param lens {size_t[]} argv 中数组元素的长度
	 * @param nchild {size_t} 有的 redis 命令需要获取多个结果集，如：subop
	 * @return {const redis_result*} 返回的结果集
	 */
	const redis_result* request(size_t argc, const char* argv[],
		size_t lens[], size_t nchild = 0);

	/**
	 * 直接组合 redis 协议命令方式，从 redis 服务器获得结果
	 * @param args {const std::vector<string>&} redis 命令组成的数组
	 * @param nchild {size_t} 有的 redis 命令需要获取多个结果集，如：subop
	 * @return {const redis_result*} 返回的结果集
	 */
	const redis_result* request(const std::vector<string>& args,
		size_t nchild = 0);

	const string* request_buf(void) const
	{
		return request_buf_;
	}

	/**
	 * 根据请求命令字列表创建 redis 请求协议数据
	 * @param argc {size_t} 命令参数个数
	 * @param argv {const char* []} 命令参数数组
	 * @param lens {size_t []} 每个命令参数长度数组
	 * @param out {string&} 存放创建结果
	 */
	static void build_request(size_t argc, const char* argv[],
		size_t lens[], string& out);

	/**
	 * 根据命令字列表创建 redis 请求协议并存储于 redis_command 中，以备在请求时使用
	 * @param argc {size_t} 命令参数个数
	 * @param argv {const char* []} 命令参数数组
	 * @param lens {size_t []} 每个命令参数长度数组
	 */
	void build_request(size_t argc, const char* argv[], size_t lens[]);

protected:
	const redis_result* run(size_t nchild = 0, int* timeout = NULL);

	void clear_request(void);
	const redis_result** scan_keys(const char* cmd, const char* key,
		int& cursor, size_t& size, const char* pattern,
		const size_t* count);
	const redis_result** scan_keys(const char* cmd, const char* key,
		size_t klen, int& cursor, size_t& size, const char* pattern,
		const size_t* count);

	/*******************************************************************/

	void build(const char* cmd, const char* key,
		const std::map<string, string>& attrs);
	void build(const char* cmd, const char* key, size_t klen,
		const std::map<string, string>& attrs);
	void build(const char* cmd, const char* key,
		const std::map<string, const char*>& attrs);

	void build(const char* cmd, const char* key,
		const std::vector<string>& names,
		const std::vector<string>& values);
	void build(const char* cmd, const char* key, size_t klen,
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
	void build(const char* cmd, const char* key, size_t klen,
		const char* names[], const size_t names_len[],
		const char* values[], const size_t values_len[], size_t argc);

	/*******************************************************************/

	void build(const char* cmd, const char* key,
		const std::vector<string>& names);
	void build(const char* cmd, const char* key, size_t klen,
		const std::vector<string>& names);
	void build(const char* cmd, const char* key,
		const std::vector<const char*>& names);
	void build(const char* cmd, const char* key,
		const std::vector<int>& names);

	void build(const char* cmd, const char* key,
		const char* names[], size_t argc);
	void build(const char* cmd, const char* key,
		const char* names[], const size_t lens[], size_t argc);
	void build(const char* cmd, const char* key, size_t klen,
		const char* names[], const size_t lens[], size_t argc);
	void build(const char* cmd, const char* key,
		const int names[], size_t argc);

	/*******************************************************************/

protected:
	int get_number(bool* success = NULL);
	long long int get_number64(bool* success = NULL);
	int get_number(std::vector<int>& out);
	int get_number64(std::vector<long long int>& out);
	bool check_status(const char* success = "OK");

	int get_status(std::vector<bool>& out);
	const char* get_status(void);

	int get_string(string& buf);
	int get_string(string* buf);
	int get_string(char* buf, size_t size);
	int get_strings(std::vector<string>& result);
	int get_strings(std::vector<string>* result);
	int get_strings(std::list<string>& result);
	int get_strings(std::list<string>* result);
	int get_strings(std::map<string, string>& result);
	int get_strings(std::vector<string>& names,
		std::vector<string>& values);
	int get_strings(std::vector<const char*>& names,
		std::vector<const char*>& values);

	/************************** common *********************************/
protected:
	dbuf_pool* dbuf_;

private:
	void init(void);

public:
	// compute hash slot of the given key and store it in the current
	// redis command will be used in the next operation for redis cluster.
	void hash_slot(const char* key);
	void hash_slot(const char* key, size_t len);

	// get the current hash slot stored internal
	int get_slot(void) const {
		return slot_;
	}

	bool is_check_addr(void) const {
		return check_addr_;
	}

protected:
	bool check_addr_;
	char addr_[32];
	redis_client* conn_;
	redis_client_cluster* cluster_;
	redis_client_pipeline* pipeline_;
	int  slot_;
	int  redirect_max_;
	int  redirect_sleep_;

public:
	const char* get_addr(const char* info);
	void set_client_addr(const char* addr);
	void set_client_addr(redis_client& conn);

public:
	redis_request* get_request_obj(void) const {
		return request_obj_;
	}

	string* get_request_buf(void) const {
		return request_buf_;
	}

	bool is_slice_req(void) const {
		return slice_req_;
	}

	// get pipline message bound with the current command
	redis_pipeline_message& get_pipeline_message(void);

protected:
	/************************** request ********************************/
	bool slice_req_;
	string* request_buf_;
	redis_request* request_obj_;
	size_t  argv_size_;
	const char**  argv_;
	size_t* argv_lens_;
	size_t  argc_;

	// reserve the argv space with the specified value at least
	void argv_space(size_t n);

	// build request in one request buffer
	void build_request1(size_t argc, const char* argv[], size_t lens[]);

	// build request with slice request obj
	void build_request2(size_t argc, const char* argv[], size_t lens[]);

protected:
	/************************** respond ********************************/
	bool slice_res_;
	redis_pipeline_message* pipe_msg_;
	const redis_result* result_;

	// save the error info into log
	void logger_result(const redis_result* result);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
