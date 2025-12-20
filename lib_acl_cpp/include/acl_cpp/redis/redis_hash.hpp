#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include <vector>
#include "../stdlib/string.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

/**
 * redis Hash (hash table) class, mainly implements the following commands:
 * redis Hash class, include commands as below:
 * HDEL/HEXISTS/HGET/HGETALL/HINCRBY/HINCRBYFLOAT/HKEYS/HLEN/HMGET/HMSET
 * HSET/HSETNX/HVALS/HSCAN
 */
class ACL_CPP_API redis_hash : virtual public redis_command {
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_hash();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_hash(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	redis_hash(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_hash(redis_client_cluster* cluster, size_t max_conns);

	redis_hash(redis_client_pipeline* pipeline);

	virtual ~redis_hash();

	/////////////////////////////////////////////////////////////////////

	/**
	 * Set multiple "field-value" pairs to the hash table corresponding to KEY.
	 * HMSET: set the key's multiple fileds in redis-server
	 * @param key {const char*} Hash table key value.
	 *  the hash key for Hash class
	 * @param attrs {const std::map<acl::string, ...>&} the fileds in map
	 * @return {bool} Whether operation was successful.
	 *  if successful for HMSET command
	 */
	bool hmset(const char* key, const std::map<string, string>& attrs);
	bool hmset(const char* key, size_t klen,
		const std::map<string, string>& attrs);
	bool hmset(const char* key, const std::map<string, const char*>& attrs);
	bool hmset(const char* key, const std::vector<string>& names,
		const std::vector<string>& values);
	bool hmset(const char* key, size_t klen,
		const std::vector<string>& names,
		const std::vector<string>& values);
	bool hmset(const char* key, const std::vector<const char*>& names,
		const std::vector<const char*>& values);
	bool hmset(const char* key, const char* names[], const char* values[],
		size_t argc);
	bool hmset(const char* key, const char* names[], const size_t names_len[],
		const char* values[], const size_t values_len[], size_t argc);
	bool hmset(const char* key, size_t klen, const char* names[],
		const size_t names_len[], const char* values[],
		const size_t values_len[], size_t argc);

	/////////////////////////////////////////////////////////////////////

	/**
	 * Get multiple "field-value" pairs from the hash table based on KEY value.
	 * get the values associated with the specified fields
	 * in the hash stored at key
	 * @param key {const char*} Hash table key value.
	 *  the hash key
	 * @param names Corresponding key field value array.
	 *  the given hash fileds
	 * @param result {std::vector<acl::string>*} When object pointer is not empty,
	 * stores query results.
	 * When this parameter is NULL, you can use base class methods like
	 * result_/get_ to get results.
	 *  store the result of the given hash files if not NULL.
	 *  If NULL, the base class's method like result_/get can be used
	 *  to get the values
	 * @return {bool} Whether operation was successful. If successful, one of the
	 * following methods can be used to get data:
	 *  if successul, one of below ways can be used to get the result:
	 *
	 *  1. Pass a non-empty storage buffer address in the call.
	 *     input the no-NULL result parameter when call hmget, when
	 *     success, the result will store the values of the given fileds
	 *
	 *  2. Call base class method result_value with specified subscript element.
	 *     call redis_command::result_value with the specified subscript
	 *
	 * 3. Call base class method result_child with specified subscript element
	 * object (redis_result), then get
	 *     element string through redis_result::argv_to_string.
	 *     call redis_command::result_child with specified subscript to
	 *     get redis_result object, then call redis_result::argv_to_string
	 *     with above result to get the values of the give fileds
	 *
	 * 4. Call base class method get_result to get the overall result object
	 * redis_result, then get
	 *     one element object through redis_result::get_child, then get the
	 *     element by the way same as the way 2 above.
	 *     call redis_command::get_result with the specified subscript to
	 *     get redis_result object, and use redis_result::get_child to
	 *     get one result object, then call redis_result::argv_to_string
	 *     to get the value of one filed.
	 *
	 * 5. Call base class method get_children to get result element array, then get
	 * each
	 * element object through redis_result's method argv_to_string to serialize
	 * element string.
	 *     use redis_command::get_children to get the redis_result array,
	 *     then use redis_result::argv_to_string to get every value of
	 *     the given fileds
	 */
	bool hmget(const char* key, const std::vector<string>& names,
		std::vector<string>* result = NULL);
	bool hmget(const char* key, size_t klen,
		const std::vector<string>& names,
		std::vector<string>* result = NULL);
	bool hmget(const char* key, const std::vector<const char*>& names,
		std::vector<string>* result = NULL);

	bool hmget(const char* key, const char* names[], size_t argc,
		std::vector<string>* result = NULL);
	bool hmget(const char* key, const char* names[], const size_t lens[],
		size_t argc, std::vector<string>* result = NULL);
	bool hmget(const char* key, size_t klen,
		const char* names[], const size_t lens[],
		size_t argc, std::vector<string>* result = NULL);

	/////////////////////////////////////////////////////////////////////

	/**
	 * Set the value of a certain field in the hash table corresponding to key.
	 * set one field's value in the hash stored at key.
	 * @param key {const char*} key value.
	 *  the hash key
	 * @param name {const char*} key corresponding field name.
	 *  the filed name of the hash key
	 * @param value {const char*} key corresponding field value.
	 *  the filed value of the hash key
	 * @return {int} Return value meaning:
	 *  1 -- Indicates a new field was added successfully.
	 *  0 -- Indicates an existing field was updated successfully.
	 * -1 -- Indicates error or key is not a hash table, operation was stopped.
	 *  return int value as below:
	 *  1 -- this is a new filed and set ok
	 *  0 -- thie is a old filed and set ok
	 * -1 -- error happend or the key is not a Hash type
	 */
	int hset(const char* key, const char* name, const char* value);
	int hset(const char* key, const char* name,
		const char* value, size_t value_len);
	int hset(const char* key, const char* name, size_t name_len,
		const char* value, size_t value_len);
	int hset(const char* key, size_t klen, const char* name,
		size_t name_len, const char* value, size_t value_len);

	/**
	 * Only update this field value when a certain field in the hash table
	 * corresponding to key does not exist.
	 * set one new field of one key in hash only when the filed isn't
	 * existing.
	 * @param key {const char*} key value.
	 *  the hash key
	 * @param name {const char*} key corresponding field name.
	 *  the field name
	 * @param value {const char*} key corresponding field value.
	 *��the field value
	 * @return {int} Return value meaning:
	 *  1 -- Indicates a new field was added successfully.
	 *  0 -- Field already exists, update was not performed.
	 * -1 -- Indicates error or key is not a hash table, operation was stopped.
	 *
	 *  return int value as below:
	 *  1 -- this is a new filed and set ok
	 *  0 -- thie is a old filed and not set
	 * -1 -- error happend or the key is not a Hash type
	 */
	int hsetnx(const char* key, const char* name, const char* value);
	int hsetnx(const char* key, const char* name,
		const char* value, size_t value_len);
	int hsetnx(const char* key, const char* name, size_t name_len,
		const char* value, size_t value_len);
	int hsetnx(const char* key, size_t klen, const char* name,
		size_t name_len, const char* value, size_t value_len);

	/**
	 * Get a certain field value from redis hash table corresponding to key.
	 * get the value assosiated with field in the hash stored at key
	 * @param key {const char*} key value.
	 *  the hash key
	 * @param name {const char*} key corresponding field name.
	 *  the field's name
	 * @param result {acl::string&} Store query result value (internally appends to
	 * this string).
	 *  store the value result of the given field
	 * @return {bool} Return value meaning:
	 * true -- Operation successful. When result is empty, it indicates KEY or
	 * field does not exist.
	 *          get the value associated with field; if result is empty then
	 *          the key or the name field doesn't exist
	 * false -- Field does not exist or operation failed or key is not a hash
	 * table.
	 *           the field not exists, or error happened,
	 *           or the key isn't a hash key
	 */
	bool hget(const char* key, const char* name, string& result);
	bool hget(const char* key, const char* name,
		size_t name_len, string& result);
	bool hget(const char* key, size_t klen, const char* name,
		size_t name_len, string& result);

	/**
	 * Get all field values from redis hash table corresponding to key.
	 * get all the fields and values in hash stored at key
	 * @param key {const char*} key value.
	 *  the hash key
	 * @param result {std::map<string, string>&} Store field name-value query
	 * results.
	 *  store the result of all the fileds and values
	 * @return {bool} Whether operation was successful. Meaning:
	 *  if ok, show below:
	 * true -- Operation successful. Even when key does not exist, it returns
	 * success. Need to check if result changed.
	 * You can determine whether query results exist by checking result.size()
	 * changes.
	 *          successful if the key is a hash key or the key not exists
	 *  false -- Operation failed or key is not a hash table.
	 *           error happened or the key isn't a hash key
	 */
	bool hgetall(const char* key, std::map<string, string>& result);
	bool hgetall(const char* key, size_t klen,
		std::map<string, string>& result);
	bool hgetall(const char* key, std::vector<string>& names,
		std::vector<string>& values);
	bool hgetall(const char* key, size_t klen,
		std::vector<string>& names, std::vector<string>& values);
	bool hgetall(const char* key, std::vector<const char*>& names,
		std::vector<const char*>& values);

	/**
	 * Delete certain fields from redis hash table corresponding to key.
	 * remove one or more fields from hash stored at key
	 * @param key {const char*} key value.
	 *  the hash key
	 * @param first_name {const char*} First field name. The last field parameter
	 * must be NULL.
	 *  the first field of the fields list, the last field must be NULL
	 *  indicating the end of vary parameters
	 * @return {int} Number of fields successfully deleted, -1 indicates error or
	 * key is not a hash table.
	 *  return the number of fields be removed successfully, or -1 when
	 *  error happened or operating on a no hash key
	 */
	int hdel(const char* key, const char* first_name);
	int hdel(const char* key, const char* names[], size_t argc);
	int hdel(const char* key, const char* names[],
		const size_t names_len[], size_t argc);
	int hdel(const char* key, size_t klen, const char* names[],
		const size_t names_len[], size_t argc);
	int hdel(const char* key, const std::vector<string>& names);
	int hdel(const char* key, size_t klen, const std::vector<string>& names);
	int hdel(const char* key, const std::vector<const char*>& names);
	int hdel_fields(const char* key, const char* names[], size_t argc);
	int hdel_fields(const char* key, const char* names[],
		const size_t names_len[], size_t argc);
	int hdel_fields(const char* key, size_t klen,
		const char* names[], const size_t names_len[], size_t argc);
	int hdel_fields(const char* key, const std::vector<string>& names);
	int hdel_fields(const char* key, size_t klen,
		const std::vector<string>& names);
	int hdel_fields(const char* key, const std::vector<const char*>& names);
	int hdel_fields(const char* key, const char* first_name, ...);

	/**
	 * When a certain field in the hash table corresponding to key is an integer,
	 * perform addition/subtraction operation.
	 * inc(+n) or dec(-n) on a integer filed in hash stored at key
	 * @param key {const char*} key value.
	 *  the hash key
	 * @param name {const char*} key corresponding field name.
	 *  the filed name of integer type
	 * @param inc {long long int} Value to add, can be negative.
	 *  the integer value to be inc or dec on the field's value
	 * @param result {long long int*} When not NULL, stores the result value.
	 *  store the result if non-NULL
	 * @return {bool} Whether operation was successful. Returns false when error or
	 * key is not a hash
	 *  table or field is not an integer type.
	 *  if successful: false when error, not a hash, or the field isn't
	 *  integer type
	 */
	bool hincrby(const char* key, const char* name,
		long long int inc, long long int* result = NULL);

	/**
	 * When a certain field in the hash table corresponding to key is a float,
	 * perform addition/subtraction operation.
	 * inc(+n) or dec(-n) on a float filed in hash stored at key
	 * @param key {const char*} key value.
	 *  the hash key
	 * @param name {const char*} key corresponding field name.
	 *  the filed name of float type
	 * @param inc {double} Value to add, can be negative.
	 *  the float value to be inc or dec on the field's value
	 * @param result {double*} When not NULL, stores the result value.
	 *  store the result if non-NULL
	 * @return {bool} Whether operation was successful. Returns false when error or
	 * key is not a hash
	 *  table or field is not a float type.
	 *  if successful: false when error, not a hash, or the field isn't
	 *  float type
	 */
	bool hincrbyfloat(const char* key, const char* name,
		double inc, double* result = NULL);

	/**
	 * Get all field names in the hash table corresponding to key.
	 * get all the fields in hash stored at key
	 * @param key {const char*} key value.
	 *  the hash key
	 * @param names {std::vector<string>&} Store all field names in the hash table
	 * corresponding to key.
	 *  store all the names of all fileds
	 * @return {bool} Whether operation was successful. Returns false when error or
	 * key is not a hash table.
	 *  return true on success, false if error happened or the
	 *  key wasn't a hash key
	 */
	bool hkeys(const char* key, std::vector<string>& names);
	bool hkeys(const char* key, size_t klen, std::vector<string>& names);

	/**
	 * Check if a certain field exists in the hash table corresponding to key.
	 * check if the field exists in hash stored at key
	 * @param key {const char*} key value.
	 *  the hash key
	 * @param name {const char*} key corresponding field name.
	 *  the filed's name of the key
	 * @return {bool} Whether operation was successful. Returns false when error or
	 * key is not a hash table
	 *  or field does not exist.
	 *  return true on success, false if error happened or the
	 *  key wasn't a hash key
	 */
	bool hexists(const char* key, const char* name);
	bool hexists(const char* key, const char* name, size_t name_len);
	bool hexists(const char* key, size_t klen, const char* name, size_t name_len);

	/**
	 * Get all field values with the specified key.
	 * get all fields' values with the specified key
	 * @param key {const char*} key value.
	 *  the hash key
	 * @param values {std::vector<string>&} Store results.
	 *  store the results
	 * @return {bool} Whether operation was successful.
	 *  return true on success, or failed when error happened
	 */
	bool hvals(const char* key, std::vector<string>& values);
	bool hvals(const char* key, size_t klen, std::vector<string>& values);

	/**
	 * Get the count of all fields in the hash table corresponding to key.
	 * get the count of fields in hash stored at key
	 * @param key {const char*} key value.
	 *  the hash key
	 * @return {int} Return value meaning:
	 *  return int value as below:
	 *  -1 -- Error or key is not a hash table.
	 *        error or not a hash key
	 *  >0 -- Field count.
	 *        the count of fields
	 *   0 -- Key does not exist or field count is 0.
	 *        key not exists or no fields in hash stored at key 
	 */
	int hlen(const char* key);
	int hlen(const char* key, size_t klen);

	/**
	 * Get the string length of the specified field value in a key.
	 * Returns the string length of the value associated with field
	 * in the hash stored at key
	 * @param key {const char*} key value.
	 *  the hash key
	 * @param name {const char*} key corresponding field name.
	 *  the field's name
	 * @return {int} If key or name does not exist, returns 0. If key is
	 *  not a hash table or error occurred, returns -1.
	 *  If the key or the field do not exist, 0 is returned; If the key is
	 *  not the hash key or error happened, -1 is returned.
	 */
	int hstrlen(const char* key, const char* name, size_t name_len);
	int hstrlen(const char* key, size_t klen, const char* name, size_t name_len);
	int hstrlen(const char* key, const char *name);
	
	/**
	 * Scan the key-value pairs in the hash table.
	 * scan the name and value of all fields in hash stored at key
	 * @param key {const char*} Hash table value.
	 *  the hash key
	 * @param cursor {int} Cursor value, write 0 at the beginning.
	 *  the cursor value, which is 0 at begin
	 * @param out {std::map<acl::string>&} Store results. Internally uses append
	 * mode to add
	 * results to the result set. To prevent the overall result from being too
	 * large, users should
	 *  clear the result set before calling this method.
	 *  store scaning result in appending mode
	 * @param pattern {const char*} Match pattern, glob style, effective only when
	 * non-empty.
	 *  match pattern, effective only on no-NULL
	 * @param count {const size_t*} Maximum count of one scan process, effective
	 * only when non-empty pointer.
	 *  the max count of one scan process, effective only on no-NULL
	 * @return {int} Next cursor position, as follows:
	 *  return the next cursor position, as below:
	 *   0: Scan finished.
	 *     scan finish
	 *  -1: Error.
	 *     some error happened
	 * >0: Next cursor position to scan. Regardless of how many results are
	 * obtained, you need to check out, as it may be empty.
	 *     the next cursor postion to scan
	 */
	int hscan(const char* key, int cursor, std::map<string, string>& out,
		const char* pattern = NULL, const size_t* count = NULL);
	int hscan(const char* key, size_t klen, int cursor,
		std::map<string, string>& out, const char* pattern = NULL,
		const size_t* count = NULL);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

