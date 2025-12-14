#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <map>
#include <list>
#include <vector>
#include "redis_result.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

class redis_request;
class redis_client;
class redis_client_cluster;
class redis_client_pipeline;
class redis_pipeline_message;

/**
 * Base virtual class for redis client command classes;
 * the redis command classes's base virtual class, which includes the basic
 * functions for all sub-classes
 */
class ACL_CPP_API redis_command : public noncopyable {
public:
	/**
	 * Default constructor. If you use this constructor to initialize the object, you must
	 * set_client or set_cluster to set the redis client communication method.
	 * default constructor. You must set the communication method by
	 * set_client or set_cluster functions.
	 */
	redis_command();

	/**
	 * When not using cluster mode, you can use this constructor to set the redis communication object.
	 * Using this constructor to set the redis communication mode,
	 * usually in no-cluster mode.
	 * @param conn {redis_client*} Redis communication object.
	 *  the redis communication in no-cluster mode
	 */
	redis_command(redis_client* conn);

	/**
	 * Constructor for cluster mode. When constructing the object, specify cluster mode.
	 * redis_client_cluster object.
	 * Using this constructor to set the redis_client_cluster, usually in
	 * cluster mode.
	 * @param cluster {redis_client_cluster*} Redis cluster connection object.
	 *  redis cluster object in cluster mode
	 *  the max of every connection pool with all the redis nodes,
	 *  if be set 0, then there is no connections limit in
	 *  connections pool.
	 */
	redis_command(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_command(redis_client_cluster* cluster, size_t max_conns);

	redis_command(redis_client_pipeline* pipeline);

	virtual ~redis_command();

	/**
	 * Whether to check socket connection address matching before processing each command.
	 * When this option is enabled, it will strictly match the connection address, which may affect performance, so it is recommended
	 * to enable this option only during DEBUG.
	 * @param on {bool}
	 */
	void set_check_addr(bool on);

	/**
	 * When reusing a redis_command subclass object to operate redis, you need to
	 * call this method first before the next call to release resources from the last operation;
	 * when reusing a redis command sub-class, the reset method should be
	 * called first to rlease some resources in last command operation
	 * @param save_slot {bool} When using cluster mode, this parameter indicates whether to save
	 *  the hash slot value. If all redis operations in the next operation use the same key value, you can
	 *  avoid recalculating the hash slot value and reduce the number of internal calculations;
	 *  when in cluster mode, if your operations is on the same key, you
	 *  can set the param save_slot to false which can reduse the times
	 *  of compute the same key's hash-slot.
	 */
	void clear(bool save_slot = false);

	ACL_CPP_DEPRECATED_FOR("clear")
	void reset(bool save_slot = false);

	/**
	 * When not using cluster mode, this function is used to set the connection obtained from the connection pool;
	 * when not using cluster mode, the function is used
	 * to set the connection for next redis command operation.
	 * @param conn {redis_client*} Redis client connection object to be set;
	 *  the redis connection to be set in next redis operation
	 */
	void set_client(redis_client* conn);

	/**
	 * Get the current redis client connection object;
	 * get redis connection set by set_client function
	 * @return {redis_client*} Returns NULL to indicate no connection object is set in the current command object
	 *  collection;
	 *  the internal redis connection be returned, NULL if no redis
	 *  connection be set 
	 */
	redis_client* get_client() const {
		return conn_;
	}

	/**
	 * Get the redis-server address bound to the current redis connection. This method can only be called
	 * when this object is redis_client (i.e., set_client) has been set.
	 * get the redis-server's addr used by the current command. this
	 * method can only be used only if the redis_client was set by
	 * set_client method.
	 * @return {const char*} Returns empty string "" to indicate no redis connection object
	 *  if "" was resturned, the redis connection was not set
	 */
	const char* get_client_addr() const;

	/**
	 * Set connection pool cluster object;
	 * set the redis cluster object in redis cluster mode
	 * @param cluster {redis_client_cluster*} Redis cluster connection object;
	 *  the redis_cluster connection object which can connect to any
	 *  redis-server and support connection pool
	 *  when dynamically creating connection pool to any redis-server, use
	 *  this param to limit the max number for each connection pool
	 */
	void set_cluster(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	void set_cluster(redis_client_cluster* cluster, size_t max_conns);

	/**
	 * Get the connection pool cluster object that was set;
	 * get redis_cluster object set by set_cluster function
	 * @return {redis_client_cluster*}
	 */
	redis_client_cluster* get_cluster() const {
		return cluster_;
	}

	/**
	 * Set pipeline communication object to use pipeline mode
	 * set the redis communication in pipeline mode
	 * @param pipeline {redis_client_pipeline*} pipeline communication object
	 */
	void set_pipeline(redis_client_pipeline* pipeline);

	/**
	 * get the redis pipeline communication object been set before
	 * @return {redis_client_pipeline*} return NULL if not set
	 */
	redis_client_pipeline* get_pipeline() const {
		return pipeline_;
	}

	/**
	 * Get memory pool handle set in memory pool redis_command internal;
	 * get memory pool handle been set
	 * @return {dbuf_pool*}
	 */
	dbuf_pool* get_dbuf() const {
		return dbuf_;
	}

	/**
	 * Get the result type returned from the current operation;
	 * get the result type returned from redis-server
	 * @return {redis_result_t}
	 */
	redis_result_t result_type() const;
	/**
	 * When the result value type is REDIS_RESULT_STATUS, the status information can be obtained;
	 * when result type is REDIS_RESULT_STATUS, the status info can be
	 * got by this function
	 * @return {const char*} Returns "" to indicate error;
	 *  "" will be returned on error
	 */
	const char* result_status() const;

	/**
	 * When the result type is REDIS_RESULT_ERROR, the error information can be obtained;
	 * when result type is REDIS_RESULT_ERROR, the error info can be
	 * get by this function
	 * @return {const char*} Returns empty string "" to indicate no error information;
	 *  "" will be returned when no error info
	 */
	const char* result_error() const;

	/**
	 * Get the number of result objects currently stored. This method can be used to get the number of elements
	 * that need to be iterated for functions (result_child/result_value);
	 * get number of result objects, just for functions
	 * result_child/result_value 
	 * @return {size_t} The correspondence between return value and result type is as follows:
	 *  the relation between return value and result type, as below:
	 *  REDIS_RESULT_ERROR: 1
	 *  REDIS_RESULT_STATUS: 1
	 *  REDIS_RESULT_INTEGER: 1
	 *  REDIS_RESULT_STRING: > 0 indicates the number of chunks the string data is divided into when the string is too large;
	 *       when the result type is REDIS_RESULT_STRING and the
	 *       string is too large, the string was being cut into many small
	 *       chunks, the returned value is the chunks number
	 *  REDIS_RESULT_ARRAY: children_->size()
	 */
	size_t result_size() const;

	/**
	 * When the result value type is REDIS_RESULT_INTEGER, get the corresponding 32-bit integer value;
	 * get 32-bits number value if result type is REDIS_RESULT_INTERGER
	 * @param success {bool*} When this pointer is not NULL, it records whether the operation was successful;
	 *  if the param pointer is not NULL, which will save status of
	 *  success or not for result from redis-server
	 * @return {int}
	 */
	int result_number(bool* success = NULL) const;

	/**
	 * When the result value type is REDIS_RESULT_INTEGER, get the corresponding 64-bit integer value;
	 * get 64-bits number value if result type is REDIS_RESULT_INTERGER
	 * @param success {bool*} When this pointer is not NULL, it records whether the operation was successful;
	 *  if the param pointer is not NULL, which will save status of
	 *  success or not for result from redis-server
	 * @return {long long int}
	 */
	long long int result_number64(bool* success = NULL) const;

	/**
	 * Get string result corresponding to subscript (when result type is REDIS_RESULT_ARRAY);
	 * get string result when result type isn't REDIS_RESULT_ARRAY
	 * @param i {size_t} Array subscript;
	 *  the array index
	 * @param len {size_t*} When not NULL pointer, stores the length of the result data;
	 *  *len will save the result's length if len is not NULL
	 * @return {const char*} Returns NULL to indicate subscript out of bounds;
	 *  NULL will return if i beyonds the array's size
	 */
	const char* get_result(size_t i, size_t* len = NULL) const;

	/**
	 * Determine whether the redis connection object (redis_client) bound to the current command has been closed.
	 * This method can only be used when conn_ object is not empty;
	 * to judge if the redis connection was being closed, only redis_client
	 * object be set internal
	 * @return {bool}
	 */
	bool eof() const;

	/**
	 * Get the result of the last redis operation;
	 * get result object of last redis operation
	 * @return {redis_result*}
	 */
	const redis_result* get_result() const;

	/**
	 * When the query result is an array type, this method gets one result element object;
	 * get one result ojbect of array if result type is REDIS_RESULT_ARRAY
	 * @param i {size_t} Result array subscript value;
	 *  the result array's index
	 * @return {const redis_result*} This method returns NULL when the result is not an array type or
	 *  array is empty or error;
	 *  NULL will be resturned when result is not REDIS_RESULT_ARRAY or
	 *  array empty or error
	 */
	const redis_result* result_child(size_t i) const;

	/**
	 * When the result obtained from redis-server is a string array type, you can call
	 * this method to get the string at a specified subscript position;
	 * when the reply from redis-serveer are strings array, this
	 * function can be used to get the string specified by a subscript
	 * @param i {size_t} Subscript (starting from 0).
	 *  the subscript of strings array
	 * @param len {size_t*} If the pointer is not empty, it stores the length of the returned result. This parameter
	 *  is only valid when the caller passes a non-empty pointer.
	 *  if len not a NULL pointer, it will store the length of string
	 *  specified by the subscript
	 * @return {const char*} Returns the value corresponding to the subscript. When NULL is returned, it means there is no
	 *  value at this subscript. To ensure usage safety, the returned string cannot guarantee ending with \0.
	 *  When calculating data length, the ending character should not be counted. To avoid binary data corruption, callers
	 *  should use the length value stored in the returned len to determine the actual length of the data.
	 *  the string will be returned associate with the subscript, if there
	 *  are nothing with the subscript, NULL will be returned
	 */
	const char* result_value(size_t i, size_t* len = NULL) const;

	/////////////////////////////////////////////////////////////////////
	/**
	 * Set whether to slice request data. When set to true, when internally packaging protocol
	 * data, it will not combine all request data into one large data packet.
	 * just for request package, setting flag for sending data with
	 * multi data chunks; this is useful when the request data is large
	 * @param on {bool} Internal default value is false.
	 *  if true the request data will not be combined one package,
	 *  internal default is false
	 */
	void set_slice_request(bool on);

	/**
	 * Set whether to slice response data. When set to true, when the server's response data
	 * is relatively large, the data will be sliced into some small data chunks.
	 * just for response package, settint flag for receiving data
	 * if split the large response data into multi little chunks
	 * @param on {bool} Internal default value is false.
	 *  if true the response data will be split into multi little
	 *  data, which is useful for large response data for avoiding
	 *  malloc large continuously memory from system.
	 *  internal default is false
	 */
	void set_slice_respond(bool on);

public:
	/**
	 * Directly call redis protocol command method to operate redis and get results.
	 * @param argc {size_t} Number of command parameter elements.
	 * @param argv {const char*[]} Redis command parameter array.
	 * @param lens {const size_t[]} Length of each argv array element.
	 * @param nchild {size_t} Some redis commands need to get multiple results, e.g., subop.
	 * @return {const redis_result*} Returned result.
	 */
	const redis_result* request(size_t argc, const char* argv[],
		const size_t lens[], size_t nchild = 0);

	/**
	 * Directly call redis protocol command method to operate redis and get results.
	 * @param args {const std::vector<string>&} Redis command parameter array.
	 * @param nchild {size_t} Some redis commands need to get multiple results, e.g., subop.
	 * @return {const redis_result*} Returned result.
	 */
	const redis_result* request(const std::vector<string>& args,
		size_t nchild = 0);

	const string* request_buf() const {
		return request_buf_;
	}

	/**
	 * Build redis request protocol from parameter list.
	 * @param argc {size_t} Number of parameters.
	 * @param argv {const char* []} Parameter array.
	 * @param lens {const size_t []} Length of each parameter string.
	 * @param out {string&} Storage buffer.
	 */
	static void build_request(size_t argc, const char* argv[],
		const size_t lens[], string& out);

	/**
	 * Build redis request protocol from parameter list and store it in redis_command for reuse.
	 * @param argc {size_t} Number of parameters.
	 * @param argv {const char* []} Parameter array.
	 * @param lens {const size_t []} Length of each parameter string.
	 */
	void build_request(size_t argc, const char* argv[], const size_t lens[]);

protected:
	const redis_result* run(size_t nchild = 0, int* timeout = NULL);

	void clear_request() const;
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
	const char* get_status();

	int get_string(string& buf);
	int get_string(string* buf);
	int get_string(char* buf, size_t size);
	int get_strings(std::vector<string>& result);
	int get_strings(std::vector<string>* result);
	int get_strings(std::list<string>& result);
	int get_strings(std::list<string>* result);
	int get_strings(std::map<string, string>& out);
	int get_strings(std::vector<string>& names,
		std::vector<string>& values);
	int get_strings(std::vector<const char*>& names,
		std::vector<const char*>& values);

	/************************** common *********************************/
protected:
	dbuf_pool* dbuf_;

private:
	void init();
	dbuf_pool *dbuf_create();

public:
	// compute hash slot of the given key and store it in the current
	// redis command will be used in the next operation for redis cluster.
	void hash_slot(const char* key);
	void hash_slot(const char* key, size_t len);

	// get the current hash slot stored internal
	int get_slot() const {
		return slot_;
	}

	bool is_check_addr() const {
		return check_addr_;
	}

protected:
	bool check_addr_;
	char addr_[128];
	redis_client* conn_;
	redis_client_cluster* cluster_;
	redis_client_pipeline* pipeline_;
	int  slot_;
	int  redirect_max_;
	int  redirect_sleep_;

public:
	const char* get_addr(const char* info);
	static const char* get_addr(dbuf_pool* dbuf, const char* info);
	void set_client_addr(const char* addr);
	void set_client_addr(redis_client& conn);

public:
	redis_request* get_request_obj() const {
		return request_obj_;
	}

	string* get_request_buf() const {
		return request_buf_;
	}

	bool is_slice_req() const {
		return slice_req_;
	}

private:
	// get pipeline message bound with the current command
	redis_pipeline_message* get_pipeline_message();

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
	void build_request1(size_t argc, const char* argv[], const size_t lens[]);

	// build request with slice request obj
	void build_request2(size_t argc, const char* argv[], const size_t lens[]);

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

