#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "../stdlib/string.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

// Data types supported by redis.
// the data type supported by redis
typedef enum {
	REDIS_KEY_NONE,		// none
	REDIS_KEY_STRING,	// string
	REDIS_KEY_HASH,		// hash
	REDIS_KEY_LIST,		// list
	REDIS_KEY_SET,		// set
	REDIS_KEY_ZSET		// sorted set
} redis_key_t;

class ACL_CPP_API redis_key : virtual public redis_command {
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_key();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_key(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	redis_key(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_key(redis_client_cluster* cluster, size_t max_conns);

	redis_key(redis_client_pipeline* pipeline);

	virtual ~redis_key();

	/**
	 * Delete one or some keys from redis. For variable args interface, the last parameter must be NULL.
	 * delete one or some keys from redis, for deleting a variable
	 * number of keys, the last key must be NULL indicating the end
	 * of the variable args
	 * @return {int} Returns number of keys deleted, as below:
	 *  0: No KEY deleted.
	 *  -1: Error occurred.
	 *  >0: Number of keys deleted. Return value may be less than actual number of keys deleted.
	 *  return the number of keys been deleted, return value as below:
	 *  0: none key be deleted
	 * -1: error happened
	 *  >0: the number of keys been deleted
	 *
	 */
	int del_one(const char* key);
	int del_one(const char* key, size_t len);
	int del(const char* key);
	int del(const std::vector<string>& keys);
	int del(const std::vector<const char*>& keys);
	int del(const char* keys[], size_t argc);
	int del(const char* keys[], const size_t lens[], size_t argc);
	int del_keys(const char* first_key, ...);
	int del_keys(const std::vector<string>& keys);
	int del_keys(const std::vector<const char*>& keys);
	int del_keys(const char* keys[], size_t argc);
	int del_keys(const char* keys[], const size_t lens[], size_t argc);

	/**
	 * Serialize object associated with key and get serialized value. RESTORE command can be used to deserialize value
	 * back to Redis.
	 * serialize the object associate with the given key, and get the
	 * value after serializing, RESTORE command can be used to
	 * deserialize by the value
	 * @param key {const char*} Key value.
	 *  the key
	 * @param len {size_t} Key length.
	 *  the key's length
	 * @param out {string&} Buffer to store serialized object result.
	 *  buffur used to store the result
	 * @return {int} Length of data after serializing.
	 *  the length of the data after serializing
	 */
	int dump(const char* key, size_t len, string& out);
	int dump(const char* key, string& out);

	/**
	 * Check if the key exists in redis.
	 * check if the key exists in redis
	 * @param key {const char*} KEY value.
	 *  the key
	 * @param len {size_t} Key length.
	 *  the key's length
	 * @return {bool} Returns true to indicate exists. Returns false to indicate error or does not exist.
	 *  true returned if key existing, false if error or not existing
	 */
	bool exists(const char* key, size_t len);
	bool exists(const char* key);

	/**
	 * Set KEY expiration time (unit: seconds).
	 * set a key's time to live in seconds
	 * @param key {const char*} Key value.
	 *  the key
	 * @param len {size_t} Key length.
	 *  the key's length
	 * @param n {int} Survival time (seconds).
	 *  lief cycle in seconds
	 * @return {int} Return value as below:
	 *  return value as below:
	 *  > 0: Successfully set expiration time.
	 *       set successfully
	 *  0: Key does not exist.
	 *    the key doesn't exist
	 *  < 0: Error occurred.
	 *       error happened
	 */
	int expire(const char* key, size_t len, int n);
	int expire(const char* key, int n);

	/**
	 * Set KEY expiration time using UNIX timestamp.
	 * set the expiration for a key as a UNIX timestamp
	 * @param key {const char*} Key value.
	 *  the key
	 * @param len {size_t} Key length.
	 *  the key's length
	 * @param stamp {time_t} UNIX timestamp, seconds since January 1, 1970.
	 *  an absolute Unix timestamp (seconds since January 1, 1970).
	 * @return {int} Return value meaning:
	 *  return value:
	 *  1: Set successfully.
	 *     the timeout was set
	 *  0: Key does not exist.
	 *     the key doesn't exist or the timeout couldn't be set
	 * -1: Error occurred.
	 *     error happened
	 */
	int expireat(const char* key, size_t len, time_t stamp);
	int expireat(const char* key, time_t stamp);

	/**
	 * Find all keys matching the given pattern.
	 * find all keys matching the given pattern
	 * @param pattern {const char*} Matching pattern.
	 *  the give matching pattern
	 * @param out {std::vector<string>*} When not NULL, stores matched results.
	 *  store the matched keys
	 * @return {int} Returns number of matched keys. 0--empty, <0 -- indicates error.
	 *  return the number of the matched keys, 0 if none, < 0 if error
	 *  Matching pattern examples:
	 *   KEYS * matches all keys in database.
	 *   KEYS h?llo matches hello, hallo, hxllo, etc.
	 *   KEYS h*llo matches hllo, heeeeello, etc.
	 *   KEYS h[ae]llo matches hello and hallo, but not hillo.
	 *
	 *  When successful, result can be obtained by one of the following ways:
	 *  1. Get specified subscript element value by base class function get_value.
	 *  2. Get specified subscript element object (redis_result) by base class function get_child, then
	 *     get element value through redis_result::argv_to_string.
	 *  3. Get overall result object redis_result by base class function get_result, then
	 *     get first element object by redis_result::get_child, then get element value by method 2
	 *     above.
	 *  4. Get child array by base class function get_children, then get element value from each
	 *     redis_result object in array through redis_result's method argv_to_string.
	 *  5. Pass non-empty storage container address in function call.
	 */
	int keys_pattern(const char* pattern, std::vector<string>* out);
	
	/**
	 * Atomically transfer a key from one redis-server to another redis-server.
	 * atomically transfer a key from a redis instance to another one
	 * @param key {const char*} Key value corresponding to data.
	 *  the key
	 * @param len {size_t} Key length.
	 *  the key's length
	 * @param addr {const char*} Destination redis-server listening address, format: ip:port
	 *  the destination redis instance's address, format: ip:port
	 * @param dest_db {unsigned} Destination database ID in destination redis-server.
	 *  the databases ID in destination redis
	 * @param timeout {unsigned} Timeout time for migration process (millisecond level)
	 *  timeout(microseconds) in transfering
	 * @param options {const char*} COPY/REPLACE/AUTH/AUTH2...
	 *  transfer option: COPY or REPLACE
	 * @return {bool} Whether migration was successful.
	 *  if transfering successfully
	 */
	bool migrate(const char* key, size_t len, const char* addr,
		unsigned dest_db, unsigned timeout, const char* options = NULL);
	bool migrate(const char* key, const char* addr, unsigned dest_db,
		unsigned timeout, const char* options = NULL);
	bool migrate(const char* addr, unsigned dest_db, unsigned timeout,
		const std::vector<const char*>& keys, std::vector<size_t>& lens,
		const char* options = NULL);

	/**
	 * Move a key to another database in the same redis-server.
	 * move a key to another database
	 * @param key {const char*} Key value.
	 *  the key
	 * @param len {size_t} Key length.
	 *  the key's length
	 * @param dest_db {unsigned} Destination database ID.
	 *  the destination database
	 * @return {int} Whether migration was successful. -1: indicates error. 0: migration failed because
	 *  same key already exists in destination database. 1: migration successful.
	 *  if moving succcessfully. -1 if error, 0 if moving failed because
	 *  the same key already exists, 1 if successful
	 */
	int move(const char* key, size_t len, unsigned dest_db);
	int move(const char* key, unsigned dest_db);

	/**
	 * Get referring count of object associated with key. This is only for debugging.
	 * get the referring count of the object, which just for debugging
	 * @param key {const char*} Key value.
	 *  the key
	 * @param len {size_t} Key length.
	 *  the key's length
	 * @return {int} Returns 0 to indicate key does not exist. < 0 to indicate error.
	 *  0 if key not exists, < 0 if error
	 */
	int object_refcount(const char* key, size_t len);
	int object_refcount(const char* key);

	/**
	 * Get internal storage encoding of object associated with key.
	 * get the internal storing of the object assosicate with the key
	 * @param key {const char*} Key value.
	 *  the key
	 * @param len {size_t} Key length.
	 * @param out {string&} Store result.
	 *  store the result
	 * @return {bool} Whether successful.
	 *  if successful
	 */
	bool object_encoding(const char* key, size_t len, string& out);
	bool object_encoding(const char* key, string& out);

	/**
	 * Get idle time (idle: not read or written) of key since first stored, unit is seconds.
	 * get the key's idle time in seconds since its first stored
	 * @param key {const char*} Key value.
	 *  the key
	 * @param len {size_t} Key length.
	 *  the key's length
	 * @return {int} Return value < 0 indicates error.
	 *  0 if error happened
	 */
	int object_idletime(const char* key, size_t len);
	int object_idletime(const char* key);

	/**
	 * Remove expiration time from key, converting key from "volatile" (key with expiration time) to
	 * "persistent" (key without expiration time, never expires).
	 * remove the expiration from a key
	 * @param key {const char*} Key value.
	 *  the key
	 * @param len {size_t} Key length.
	 *  the key's length
	 * @return {int} Return value meaning:
	 *  the value returned as below:
	 *  1 -- Set successfully.
	 *       set ok
	 *  0 -- Key does not exist or expiration time was not set.
	 *       the key not exists or not be set expiration
	 * -1 -- Error occurred.
	 *       error happened
	 */
	int persist(const char* key, size_t len);
	int persist(const char* key);

	/**
	 * Set KEY expiration time (unit: milliseconds).
	 * set a key's time to live in milliseconds
	 * @param key {const char*} Key value.
	 *  the key
	 * @param len {size_t} Key length.
	 *  the key's length
	 * @param n {int} Survival time (milliseconds).
	 *  time to live in milliseconds
	 * @return {int} Return value as below:
	 *  value returned as below:
	 *  > 0: Successfully set expiration time.
	 *       set successfully
	 *    0: Key does not exist.
	 *       the key doesn't exist
	 *  < 0: Error occurred.
	 *       error happened
	 */
	int pexpire(const char* key, size_t len, int n);
	int pexpire(const char* key, int n);

	/**
	 * Set key expiration time using UNIX timestamp in milliseconds.
	 * set the expiration for a key as UNIX timestamp specified
	 * in milliseconds
	 * @param key {const char*} Key value.
	 *  the key
	 * @param len {size_t} Key length.
	 *  the key's length
	 * @param n {long long int} UNIX timestamp, milliseconds since January 1, 1970.
	 *  the UNIX timestamp in milliseconds from 1970.1.1
	 * @return {int} Return value as below:
	 *  value resturned as below:
	 *  > 0: Successfully set expiration time.
	 *       set successfully
	 *    0: Key does not exist.
	 *       the key doesn't exist
	 *  < 0: Error occurred.
	 *       error happened
	 */
	int pexpireat(const char* key, size_t len, long long int n);
	int pexpireat(const char* key, long long int n);

	/**
	 * Get KEY remaining survival time (unit: milliseconds).
	 * get the time to live for a key in milliseconds
	 * @param key {const char*} Key value.
	 *  the key
	 * @param len {size_t} Key length.
	 *  the key's length
	 * @return {int} Returns corresponding value, as below:
	 *  value returned as below:
	 *  >0: Key remaining survival time (milliseconds).
	 *      the time to live for a key in milliseconds
	 *  -3: Error occurred.
	 *      error happened
	 *  -2: Key does not exist.
	 *      the key doesn't exist
	 *  -1: Key exists but has no remaining survival time.
	 *      th key were not be set expiration
	 * Note: For redis-server versions before 2.8, -1 will be returned if key
	 * does not exist or key was not set expiration.
	 * notice: for redis version before 2.8, -1 will be returned if the
	 * key doesn't exist or the key were not be set expiration.
	 */
	long long int pttl(const char* key, size_t len);
	long long int pttl(const char* key);

	/**
	 * Return (but not delete) a random key from current database.
	 * return a random key from the keyspace
	 * @param buf {string&} Store result when KEY is successfully obtained.
	 *  store the key
	 * @return {bool} Whether successful. Returns false when key space is empty.
	 *  true on success, or false be returned
	 */
	bool randomkey(string& buf);

	/**
	 * Rename key to newkey.
	 * rename a key
	 * @return {bool}
	 *  true on success, or error happened
	 */
	bool rename_key(const char* key, const char* newkey);

	/**
	 * Rename key to newkey only if newkey does not exist.
	 * rename a key only if the new key does not exist
	 * @param key {const char*} Old key.
	 * @param newkey {const char*} New key.
	 * @return {int} Return value > 0: success. 0: target key exists. < 0: failure.
	 *  return value > 0 on success, < 0 on error, == 0 when newkey exists
	 */
	int renamenx(const char* key, const char* newkey);

	/**
	 * Create a key using provided serialized value, previously obtained by using DUMP.
	 * create a key using the provided serialized value, previously
	 * obtained by using DUMP
	 * @param ttl {int} Survival time for key in milliseconds. If ttl is 0,
	 *  then expiration will not be set.
	 *  the time to live for the key in milliseconds, if tll is 0,
	 *  expiration will not be set
	 * @param replace {bool} If key already exists, whether to directly replace.
	 *  if the key already exists, this parameter decides if replacing
	 *  the existing key
	 * @return {bool}
	 *  true on success, false on error
	 */
	bool restore(const char* key, const char* value, size_t len,
		int ttl, bool replace = false);

	/**
	 * Get KEY remaining survival time (unit: seconds).
	 * get the time to live for a key in seconds
	 * @param key {const char*} Key value.
	 *  the key
	 * @param len {size_t} Key length.
	 *  the key's length
	 * @return {int} Returns corresponding value, as below:
	 *  return value as below:
	 *  > 0: Key remaining survival time (seconds).
	 *       the time to live for a key in seconds
	 *   -3: Error occurred.
	 *       error happened
	 *   -2: Key does not exist.
	 *       the key doesn't exist
	 *   -1: Key exists but has no remaining survival time.
	 *       the key were not be set expiration
	 * Note: For redis-server versions before 2.8, -1 will be returned
	 *  if key does not exist or key was not set expiration.
	 * notice: for the redis version before 2.8, -1 will be returned
	 *  if the key doesn't exist or the key were not be set expiration
	 */
	int ttl(const char* key, size_t len);;
	int ttl(const char* key);

	/**
	 * Get storage type of KEY.
	 * get the the type stored at key
	 * @para key {const char*} KEY value.
	 *  the key
	 * @param len {size_t} Key length.
	 *  the key's length
	 * @return {redis_key_t} Returns storage type of KEY.
	 *  return redis_key_t defined above as REDIS_KEY_
	 */
	redis_key_t type(const char* key, size_t len);
	redis_key_t type(const char* key);

	/**
	 * Incrementally iterate the keys space in the specified database.
	 * incrementally iterate the keys space in the specified database
	 * @param cursor {int} Cursor value. Write 0 when starting iteration.
	 *  the iterating cursor beginning with 0
	 * @param out {std::vector<acl::string>&} String array storing results. Array will be cleared
	 *  internally and string results will be appended to array. To prevent results from growing too large, users should
	 *  clear this parameter before each iteration.
	 *  string array storing the results, the array will be cleared
	 *  internal and the string result will be appened to the array
	 * @param pattern {const char*} Matching pattern with glob style, effective when not empty.
	 *  the matching pattern with glob style, only effective if not NULL
	 * @param count {const size_t*} Limit maximum number of results stored in array, effective
	 *  only when not NULL.
	 *  limit the max number of the results stored in array, only
	 *  effective when not NULL
	 * @return {int} Next cursor position, as below:
	 *  return the next cursor value as follow:
	 *   0: Iteration is finished. At this time, you need to check if out results are empty. If
	 *      not empty, you need to process them.
	 *      iterating is finished and the out should be checked if emtpy
	 *  -1: Error occurred.
	 *      some error happened
	 *  >0: Next cursor position. Regardless of how many results are obtained, you need to check out, as it may be empty.
	 *      the next cursor value for iterating
	 */
	int scan(int cursor, std::vector<string>& out,
		const char* pattern = NULL, const size_t* count = NULL);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

