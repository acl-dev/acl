#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include <vector>
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

/**
 * All string commands in redis are implemented.
 * all the commands in redis Strings are be implemented.
 */
class ACL_CPP_API redis_string : virtual public redis_command {
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_string();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	explicit redis_string(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	explicit redis_string(redis_client_cluster* cluster);

	explicit redis_string(redis_client_pipeline* pipeline);

	ACL_CPP_DEPRECATED
	redis_string(redis_client_cluster* cluster, size_t max_conns);

	virtual ~redis_string();

	/////////////////////////////////////////////////////////////////////
	
	/**
	 * Set string value value to key.
	 * set the string value of a key
	 * @param key {const char*} String object's key.
	 *  the key of a string
	 * @param value {const char*} String object's value.
	 *  the value of a string
	 * @return {bool} Whether operation was successful. Returns false to indicate error occurred or key is not a string object.
	 *  true if SET was executed correctly, false if error happened or
	 *  the key's object isn't a string.
	 */
	bool set(const char* key, const char* value);
	bool set(const char* key, size_t key_len,
		const char* value, size_t value_len);

	#define SETFLAG_EX 	0x02
	#define SETFLAG_PX	0x03
	#define SETFLAG_NX	0x08
	#define SETFLAG_XX	0x0C
	/**
	 * Starting from Redis 2.6.12 version, SET command can be modified through a series of parameters:
	 * EX seconds sets key expiration time to seconds seconds. Executing SET key value
	 * EX seconds has the same effect as executing SETEX key seconds value.
	 * PX milliseconds sets key expiration time to milliseconds milliseconds.
	 *   Executing SET key value PX milliseconds has the same effect as executing
	 *     PSETEX key milliseconds value.
	 * NX only sets key value when key does not exist. Executing SET key value NX has the
	 *  same effect as executing SETNX key value.
	 * XX only sets key value when key already exists.
	 * @Note Because SET command can achieve SETNX and SETEX as well as PSETEX
	 * command effects through parameters, future Redis versions may deprecate SETNX, SETEX
	 * and PSETEX commands.
	 * @param key {const char*} String object's key.
	 *  the key of a string
	 * @param value {const char*} String object's value.
	 *  the value of a string
	 * @param timeout {int} Expiration value, unit is seconds (EX)/milliseconds (PX)
	 *  the timeout in seconds of a string
	 * @param flag {int} Flag: EX/PX | NX/XX
	 *  the flag of a string
	 * @return {bool} Whether operation was successful. Returns false to indicate error occurred or key is not a string object.
	 *  true if SET was executed correctly, false if error happened or
	 *  the key's object isn't a string.
	 */
	bool set(const char* key, const char* value, int timeout, int flag);
	bool set(const char* key, size_t key_len, const char* value,
		size_t value_len, int timeout, int flag);

	/**
	 * Set value value to key, and set key expiration time to timeout (unit: seconds).
	 * If key already exists, SETEX command will overwrite value.
	 * set key to hold the strnig value, and set key to timeout after
	 * a given number of seconds.
	 * @param key {const char*} String object's key.
	 *  the key of a string
	 * @param value {const char*} String object's value.
	 *  the value of a string
	 * @param timeout {int} Expiration value, unit is seconds.
	 *  the timeout in seconds of a string
	 * @return {bool} Whether operation was successful. Returns false to indicate error occurred or key is not a string object.
	 *  true if SETEX was executed correctly, false if error happened
	 *  or the object specified by the key is not a string
	 */
	bool setex(const char* key, const char* value, int timeout);
	bool setex(const char* key, size_t key_len, const char* value,
		size_t value_len, int timeout);

	/**
	 * Set value value to key, and set key expiration time to timeout (unit: milliseconds).
	 * If key already exists, SETEX command will overwrite value.
	 * set key to hold the string value, and set key to timeout after
	 * a given number of milliseconds.
	 * @param key {const char*} String object's key.
	 *  the key of a string
	 * @param value {const char*} String object's value.
	 *  the value of a string
	 * @param timeout {int} Expiration value, unit is milliseconds.
	 *  the timeout in milliseconds of a string
	 * @return {bool} Whether operation was successful. Returns false to indicate error occurred or key is not a string object.
	 *  true if SETEX was executed correctly, false if error happened
	 *  or the object specified by the key is not a string
	 */
	bool psetex(const char* key, const char* value, int timeout);
	bool psetex(const char* key, size_t key_len, const char* value,
		size_t value_len, int timeout);

	/**
	 * Set key value to value, only if key does not exist. If key already exists,
	 * SETNX command does nothing.
	 * set the value of a key, only if the key does not exist.
	 * @param key {const char*} String object's key.
	 *  the key of the string
	 * @param value {const char*} String object's value.
	 *  the value of the string
	 * @return {int} Return value as below:
	 *  return the value as below:
	 *  -1: Error occurred or key is not a string object.
	 *      error happened or the object by the key isn't a string
	 *   0: Key object already exists.
	 *      the string of the key already exists
	 *   1: Operation successful.
	 *      the command was executed correctly
	 */
	int setnx(const char* key, const char* value);
	int setnx(const char* key, size_t key_len,
		const char* value, size_t value_len);

	/**
	 * If key already exists and is a string, APPEND command will append value to key's original
	 * value end. If key does not exist, APPEND simply sets key to value.
	 * append a value to a key
	 * @param key {const char*} String object's key.
	 *  the key of a string
	 * @param value {const char*} String value to be appended.
	 *  the value to be appended to a key
	 * @return {int} Returns current string length after appending. -1 indicates error occurred or key is not a string object.
	 *  return the length of the string after appending, -1 if error
	 *  happened or the key's object isn't a string
	 */
	int append(const char* key, const char* value);
	int append(const char* key, const char* value, size_t size);

	/**
	 * Get string value stored at key.
	 * get the value of a key 
	 * @param key {const char*} String object's key.
	 *  the key of a string
	 * @param buf {string&} When successful, stores string object's value. When returns true,
	 *  if buf is empty, it indicates corresponding key does not exist.
	 *  store the value of a key after GET executed correctly, key not
	 *  exist if the buf is empty when return true
	 * @return {bool} Whether operation was successful. Returns false to indicate error occurred or key is not a string object.
	 *  if the GET was executed correctly, false if error happened or
	 *  is is not a string of the key
	 */
	bool get(const char* key, size_t len, string& buf);
	bool get(const char* key, string& buf);

	/**
	 * Get string value stored at key. When returned string value is relatively large, internally automatically uses slice method to return
	 * data in memory chunks to save memory. Users need to call redis_result::get(size_t, size_t*) to get a certain
	 * slice's data, and call redis_result::get_size() to get total slice length.
	 * @param key {const char*} String object's key.
	 * @return {bool} Whether operation was successful. Returns false to indicate error occurred or key is not a string object.
	 */
	const redis_result* get(const char* key);
	const redis_result* get(const char* key, size_t len);

	/**
	 * Set key value to value, and return key's old value. When key exists but is not
	 * a string, an error will occur.
	 * set the string value of a key and and return its old value
	 * @param key {const char*} String object's key.
	 *  the key of string
	 * @param value {const char*} New object value to be set.
	 *  the new string value of the key
	 * @param buf {string&} Store returned old value.
	 *  store the old string value of the key
	 * @return {bool} Whether successful.
	 *  if GETSET was executed correctly.
	 */
	bool getset(const char* key, const char* value, string& buf);
	bool getset(const char* key, size_t key_len, const char* value,
		size_t value_len, string& buf);

	/////////////////////////////////////////////////////////////////////

	/**
	 * Get length of value data stored at specified key.
	 * get the length of value stored in a key
	 * @param key {const char*} String object's key.
	 *  the key of the string
	 * @return {int} Return value as below:
	 *  return value as below:
	 *  -1: Error occurred or it is not a string object.
	 *      error happened or the it isn't a string of the key
	 *   0: Key does not exist.
	 *      the key doesn't exist
	 *  >0: String data length.
	 *      the length of the value stored in a key
	 */
	int get_strlen(const char* key);
	int get_strlen(const char* key, size_t key_len);

	/**
	 * Overwrite (overwrite) part of string value stored at key starting at offset.
	 * Non-existent key will be padded with null characters.
	 * overwrite part of a string at key starting at the specified offset
	 * @param key {const char*} String object's key.
	 *  the key of a string
	 * @param offset {unsigned} Offset start position. This value can exceed string data length. At this time
	 *  null characters \0 are padded.
	 *  the specified offset of the string
	 * @param value {const char*} Value to be set.
	 *  the value to be set
	 * @return {int} Current string data length after operation.
	 *  the length of the string after SETRANGE
	 */
	int setrange(const char* key, unsigned offset, const char* value);
	int setrange(const char* key, size_t key_len, unsigned offset,
		const char* value, size_t value_len);

	/**
	 * Get substring of string value stored at key. Substring range is from start to end offset positions
	 * (including start and end).
	 * get substring of the string stored at a key
	 * @param key {const char*} String object's key.
	 *  the key of string
	 * @param start {int} Start subscript value.
	 *  the starting offset of the string
	 * @param end {int} End subscript value.
	 *  the ending offset of the string
	 * @param buf {string&} Store result when successful.
	 *  store the substring result
	 * @return {bool} Whether operation was successful.
	 *  if GETRANGE was executed correctly.
	 *  Note: Subscript position can be negative, indicating counting from string end forward. For example, -1 indicates last element.
	 */
	bool getrange(const char* key, int start, int end, string& buf);
	bool getrange(const char* key, size_t key_len,
		int start, int end, string& buf);

	/////////////////////////////////////////////////////////////////////

	/**
	 * Set or clear bit at offset in string value stored at key.
	 * Bit value can be set by user to 0 or 1.
	 * set or clear the bit at offset in the string value stored at key
	 * @param key {const char*} String object's key.
	 *  the key of the string
	 * @param offset {unsigned} Specified offset bit position.
	 *  the offset at the string value
	 * @param bit {bool} When true, sets flag bit. When false, clears flag bit.
	 *  set bit if true, or clear bit if false at the specified offset
	 * @return {bool} Whether operation was successful.
	 *  if the command was executed correctly
	 */
	bool setbit_(const char* key, unsigned offset, bool bit);
	bool setbit_(const char* key, size_t len, unsigned offset, bool bit);

	/**
	 * Get bit at offset in string value stored at key. When offset exceeds string value
	 * length, or key does not exist, returns 0.
	 * get the bit at offset in the string value stored at key
	 * @param key {const char*} String object's key.
	 *  the key of the string
	 * @param offset {unsigned} Specified offset bit position.
	 *  the offset in the string value
	 * @param bit {int&} Stores flag bit at specified position when successful.
	 *  on success it will stored the bit at the specified offset
	 * @return {bool} Whether operation was successful. Returns false to indicate error occurred or key is not a string object.
	 *  if the GETBIT was executed correctly, false if error happened,
	 *  or the key doesn't store a string object
	 */
	bool getbit(const char* key, unsigned offset, int& bit);
	bool getbit(const char* key, size_t len, unsigned offset, int& bit);

	/**
	 * Count number of bits set to 1 in string. When start/end are specified, only counts within specified range.
	 * count set bits in a string
	 * @param key {const char*} String object's key.
	 *  the key of a string
	 * @return {int} Number of bits set to 1. -1 indicates error occurred or it is not a string object.
	 *  the count of bits been set, -1 if error happened or it's not
	 *  a string
	 */
	int bitcount(const char* key);
	int bitcount(const char* key, size_t len);
	int bitcount(const char* key, int start, int end);
	int bitcount(const char* key, size_t len, int start, int end);

	/**
	 * Perform logical AND operation on multiple source keys and save result to destkey.
	 * BITOP AND on multiple source keys and save the result to another key
	 * @param destkey {const char*} Destination string object's key.
	 *  the key storing the result
	 * @param keys Source string object collection.
	 *  the source keys
	 * @return {int} Length of string stored in destination key.
	 *  the size of the string stored in the destination key, that is
	 *  equal to the size of the longest input string
	 */
	int bitop_and(const char* destkey, const std::vector<string>& keys);
	int bitop_and(const char* destkey, const std::vector<const char*>& keys);
	int bitop_and(const char* destkey, const char* key, ...);
	int bitop_and(const char* destkey, const char* keys[], size_t size);

	/**
	 * Perform logical OR operation on multiple source keys and save result to destkey.
	 * BITOP OR on multiple source keys and save the result to another key
	 * @param destkey {const char*} Destination string object's key.
	 *  the destination key
	 * @param keys Source string object collection.
	 *  the source keys
	 * @return {int}
	 *  the size of the string stored in the destination key
	 */
	int bitop_or(const char* destkey, const std::vector<string>& keys);
	int bitop_or(const char* destkey, const std::vector<const char*>& keys);
	int bitop_or(const char* destkey, const char* key, ...);
	int bitop_or(const char* destkey, const char* keys[], size_t size);

	/**
	 * Perform logical XOR operation on multiple source keys and save result to destkey.
	 * BITOP XOR on multiple source keys and save the result to another key
	 * @param destkey {const char*} Destination string object's key.
	 *  the destination key
	 * @param keys Source string object collection.
	 *  the source keys
	 * @return {int}
	 *  the size of the string stored in the destination key
	 */
	int bitop_xor(const char* destkey, const std::vector<string>& keys);
	int bitop_xor(const char* destkey, const std::vector<const char*>& keys);
	int bitop_xor(const char* destkey, const char* key, ...);
	int bitop_xor(const char* destkey, const char* keys[], size_t size);

	/////////////////////////////////////////////////////////////////////

	/**
	 * Set multiple key-value pairs simultaneously.
	 * set multiple key-value pair
	 * @param objs key-value pair collection.
	 *  the collection of multiple key-value pair
	 * @return {bool} Whether operation was successful.
	 *  if the command was executed correctly
	 */
	bool mset(const std::map<string, string>& objs);
	bool mset(const std::vector<string>& keys,
		const std::vector<string>& values);
	bool mset(const char* keys[], const char* values[], size_t argc);
	bool mset(const char* keys[], const size_t keys_len[],
		const char* values[], const size_t values_len[], size_t argc);

	/////////////////////////////////////////////////////////////////////

	/**
	 * Set multiple keys to multiple values only if none of the keys exist.
	 * set multiple keys to multiple values only if none of the keys exist
	 * @param objs key-value pair collection.
	 *  the collection of multile key-value pair
	 * @return {int} Return value as below:
	 *  return value as below:
	 *  -1: Error occurred or there is an object that is not a string.
	 *      error happened or there were a object of not a string.
	 *   0: None were set because some of the keys already exist.
	 *     none be set because some of the keys already exist
	 *   1: Operation successful.
	 *     add ok.
	 */
	int msetnx(const std::map<string, string>& objs);
	int msetnx(const std::vector<string>& keys,
		const std::vector<string>& values);
	int msetnx(const char* keys[], const char* values[], size_t argc);
	int msetnx(const char* keys[], const size_t keys_len[],
		const char* values[], const size_t values_len[], size_t argc);

	/////////////////////////////////////////////////////////////////////

	/**
	 * Get values of given keys (one or multiple). When all keys exist, returns values. If a certain key does not exist,
	 * then that key returns empty string. Result array order corresponds to key order.
	 * get the values of the given keys
	 * @param keys {const std::vector<string>&} String key array.
	 *  the given keys
	 * @param out {std::vector<acl::string>*} When not empty, stores string value result array.
	 *  Internally, non-existent keys also store an empty string.
	 *  acl::string array storing the result. if one key not exists,
	 *  a empty string "" will also be stored in the array.
	 * @return {bool} Whether operation was successful. When successful, result can be obtained by one of the following ways:
	 *  if successul, one of below ways can be used to get the result:
	 *
	 *  1. Pass non-empty storage container address in function call.
	 *     input the no-NULL result parameter when call hmget, when
	 *     success, the result will store the values of the given fileds
	 *
	 *  2. Get specified subscript element value by base class function get_value.
	 *     call redis_command::result_value with the specified subscript
	 *
	 *  3. Get specified subscript element object (redis_result) by base class function get_child, then get
	 *     element value through redis_result::argv_to_string.
	 *     redis_result::argv_to_string to get element value.
	 *     call redis_command::result_child with specified subscript to
	 *     get redis_result object, then call redis_result::argv_to_string
	 *     with above result to get the values of the give fileds
	 *
	 *  4. Get overall result object redis_result by base class function get_result, then get
	 *     first element object by redis_result::get_child, then get element value by method 2 above.
	 *     call redis_command::get_result with the specified subscript to
	 *     get redis_result object, and use redis_result::get_child to
	 *     get one result object, then call redis_result::argv_to_string
	 *     to get the value of one filed.
	 *
	 *  5. Get child array by base class function get_children, then get element value from each
	 *     redis_result object in array through redis_result's method argv_to_string.
	 *     use redis_command::get_children to get the redis_result array,
	 *     then use redis_result::argv_to_string to get every value of
	 *     the given fileds
	 */
	bool mget(const std::vector<string>& keys,
		std::vector<string>* out = NULL);
	bool mget(const std::vector<const char*>& keys,
		std::vector<string>* out = NULL);

	bool mget(std::vector<string>* result, const char* first_key, ...);
	bool mget(const char* keys[], size_t argc,
		std::vector<string>* out = NULL);
	bool mget(const char* keys[], const size_t keys_len[], size_t argc,
		std::vector<string>* out = NULL);

	/////////////////////////////////////////////////////////////////////

	/**
	 * Increment integer value stored at key by one.
	 * 1. If key does not exist, key value will be initialized to 0 first, then INCR operation is executed.
	 * 2. If value is not a number type, i.e., string type value cannot be represented as a number, then an error occurs.
	 * 3. Integer value must be within 64-bit (bit) signed integer representation range.
	 * increment the integer value of a key by one
	 * 1) if key not exists, the key's value will be set 0 and INCR
	 * 2) if key's value is not a number an error will be returned
	 * 3) the number is a 64 signed integer
	 * @param key {const char*} String object's key.
	 *  the given key
	 * @param result {long long int*} When not empty, stores operation result.
	 *  store the result after INCR if it isn't NULL
	 * @return {bool} Whether operation was successful.
	 *  if the INCR was executed correctly
	 */
	bool incr(const char* key, long long int* result = NULL);

	/**
	 * Increment integer value stored at key by increment.
	 * 1. If key does not exist, key value will be initialized to 0 first, then INCRBY operation is executed.
	 * 2. If value is not a number type, i.e., string type value cannot be represented as a number, then an error occurs.
	 * 3. Integer value must be within 64-bit (bit) signed integer representation range.
	 * increment the integer value of a key by a given amount
	 * 1) if key not exists, the key's value will be set 0 and INCRBY
	 * 2) if key's value is not a number an error will be returned
	 * 3) the number is a 64 signed integer
	 * @param key {const char*} String object's key.
	 *  the given key
	 * @param inc {long long int} Increment value.
	 *  the given amount
	 * @param result {long long int*} When not empty, stores operation result.
	 *  store the result after INCR if it isn't NULL
	 * @return {bool} Whether operation was successful.
	 *  if the INCRBY was executed correctly
	 */
	bool incrby(const char* key, long long int inc,
		long long int* result = NULL);

	/**
	 * Increment float value stored at key by given increment.
	 * 1) If key does not exist, INCRBYFLOAT first sets key value to 0, then executes addition operation.
	 * 2) If operation executes successfully, key value will be updated to result after addition (float value). Return value
	 *    is returned in string format.
	 * 3) Float value can only represent up to 17 decimal places.
	 * increment the float value of a key by the given amount
	 * 1) if key not exists, the key's value will be set 0 and INCRBYFLOAT
	 * 2) if key's value is not a float an error will be returned
	 * @param key {const char*} String object's key.
	 *  the given key
	 * @param inc {double} Increment value.
	 *  the given amount
	 * @param result {double*} When not empty, stores operation result.
	 *  store the result after INCR if it isn't NULL
	 * @return {bool} Whether operation was successful.
	 *  if the INCRBYFLOAT was executed correctly
	 */
	bool incrbyfloat(const char* key, double inc, double* result = NULL);

	/**
	 * Decrement integer value stored at key by one.
	 * 1) If key does not exist, key value will be initialized to 0 first, then DECR operation is executed.
	 * 2) If value is not a number type, i.e., string type value cannot be represented as a number, then an error occurs.
	 * 3) Integer value must be within 64-bit (bit) signed integer representation range.
	 * decrement the integer value of a key by one
	 * 1) if key not exists, the key's value will be set 0 and DECR
	 * 2) if key's value is not a number an error will be returned
	 * 3) the number is a 64 signed integer
	 * @param key {const char*} String object's key.
	 *  the given key
	 * @param result {long long int*} When not empty, stores operation result.
	 *  store the result after INCR if it isn't NULL
	 * @return {bool} Whether operation was successful.
	 *  if the DECR was executed correctly
	 */
	bool decr(const char* key, long long int* result = NULL);

	/**
	 * Decrement integer value stored at key by decrement.
	 * 1) If key does not exist, key value will be initialized to 0 first, then DECRBY operation is executed.
	 * 2) If value is not a number type, i.e., string type value cannot be represented as a number, then an error occurs.
	 * 3) Integer value must be within 64-bit (bit) signed integer representation range.
	 * decrement the integer value of a key by the given amount
	 * @param key {const char*} String object's key.
	 *  the given key
	 * @param dec {long long int} Decrement value.
	 *  the given amount
	 * @param result {long long int*} When not empty, stores operation result.
	 *  store the result after INCR if it isn't NULL
	 * @return {bool} Whether operation was successful.
	 *  if the DECRBY was executed correctly
	 */
	bool decrby(const char* key, long long int dec,
		long long int* result = NULL);

private:
	int bitop(const char* op, const char* destkey,
		const std::vector<string>& keys);
	int bitop(const char* op, const char* destkey,
		const std::vector<const char*>& keys);
	int bitop(const char* op, const char* destkey,
		const char* keys[], size_t size);

	bool incoper(const char* cmd, const char* key, long long int* inc,
		long long int* result);

};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

