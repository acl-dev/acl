#pragma once
#include "../acl_cpp_define.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

class ACL_CPP_API redis_list : virtual public redis_command {
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_list(void);

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_list(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	redis_list(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_list(redis_client_cluster* cluster, size_t max_conns);

	redis_list(redis_client_pipeline* pipeline);

	virtual ~redis_list(void);

	/////////////////////////////////////////////////////////////////////

	/**
	 * Remove and get an element from list head at key, or block until one
	 * is available; when multiple keys were given, multiple elements
	 * will be gotten according the sequence of keys given.
	 * remove and get a element from list head, or block until one
	 * is available; when multiple keys were given, multiple elements
	 * will be gotten according the sequence of keys given.
	 * @param result {std::pair<string, string>&} Store element pair result. This pair's
	 *  first string indicates list's key, second is element popped from head.
	 *  store the elements result, the first string of pair is the key,
	 *  and the second string of pair is the element
	 * @param timeout {size_t} Blocking timeout in seconds before one element available. When timeout expires and no element pair is
	 *  available, returns false. When this value is 0, waits indefinitely until element pair is available.
	 *  the blocking timeout in seconds before one element availble;
	 *  false will be returned when the timeout is arrived; if the timeout
	 *  was set to be 0, this function will block until a element was
	 *  available or some error happened.
	 * @param first_key {const char*} First non-empty string key. The last parameter must
	 *  be NULL, indicating end of variable args.
	 *  the first key of a variable args, the last arg must be NULL
	 *  indicating the end of the variable args.
	 * @return {bool} Whether got element pair from head. When false, possible reasons:
	 *  true if got a element in the head of list, when false was be
	 *  returned, there'are some reasons show below:
	 *  1. Error occurred.
	 *     error happened.
	 *  2. At least one key is not a list object.
	 *     at least one key was not a list object.
	 *  3. Key does not exist or timeout occurred.
	 *     key not exist or timeout was got.

	 */
	bool blpop(std::pair<string, string>& result, size_t timeout,
		const char* first_key, ...);
	bool blpop(const std::vector<const char*>& keys, size_t timeout,
		std::pair<string, string>& result);
	bool blpop(const std::vector<string>& keys, size_t timeout,
		std::pair<string, string>& result);

	/**
	 * Same meaning as blpop above, except that this function
	 * is used to pop element from the tail of the list.
	 * the meaning is same as the blpop above except that this function
	 * is used to pop element from the tail of the list
	 * @see blpop
	 */
	bool brpop(std::pair<string, string>& result, size_t timeout,
		const char* first_key, ...);
	bool brpop(const std::vector<const char*>& keys, size_t timeout,
		std::pair<string, string>& result);
	bool brpop(const std::vector<string>& keys, size_t timeout,
		std::pair<string, string>& result);

	/**
	 * Execute two actions in blocking mode as below:
	 * 1) Pop an element from src list's tail, and return it to caller.
	 * 2) Push the element to dst list's head.
	 * two actions will be executed in blocking mode as below:
	 * 1) pop a element from src list's tail, and return it to caller
	 * 2) push the element to dst list's head
	 * @param src {const char*} Source list's key.
	 *  the key of source list
	 * @param dst {const char*} Destination list's key.
	 *  the key of destination list
	 * @param buf {string*} When not empty, stores element popped from src tail.
	 *  if not NULL, buf will store the element poped from the head of src
	 * @param timeout {size_t} Wait timeout. When 0, waits indefinitely until data is available or
	 *  error occurs.
	 *  the timeout to wait, if the timeout is 0 this function will
	 *  block until a element was available or error happened.
	 * @return {bool} When successfully popped element from src list tail and pushed to dst list head,
	 *  this function returns true. Returns false to indicate timeout, error occurred, or one of src/dst is not a list object.
	 *  true if success, false if timeout arrived, or error happened,
	 *  or one of the src and dst is not a list object
	 * @see rpoplpush
	 */
	bool brpoplpush(const char* src, const char* dst, size_t timeout,
		string* buf = NULL);

	/**
	 * Return the element of the specified subscript from the list at key.
	 * return the element of the specified subscript from the list at key
	 * @param key {const char*} List object's key.
	 *  the key of one list object
	 * @param idx {size_t} Subscript value.
	 *  the specified subscript
	 * @param buf {string&} Store result.
	 *  store the result
	 * @return {bool} When returns true, if successful, buf data is non-empty, indicating correctly got
	 *  element at specified subscript. If buf.empty(), it means no element was got. When false, error occurred.
	 *  true if success, and if buf is empty, no element was got;
	 *  false if error happened
	 */
	bool lindex(const char* key, size_t idx, string& buf);

	/**
	 * Insert one new element before the specified element in list.
	 * insert one new element before the specified element in list
	 * @param key {const char*} List object's key.
	 *  the key of specified list
	 * @param pivot {const char*} A specified element in list.
	 *  the speicifed element of list
	 * @param value {const char*} New element value.
	 *  the new element to be inserted
	 * @reutrn {int} Returns number of elements in list specified by key, as below:
	 *  return the number of list specified by the given key, as below:
	 *  -1 -- Indicates error occurred or key is not a list object.
	 *        error happened or the object of the key is not a list
	 *  >0 -- Current list's element count.
	 *        the number of elements of the specified list
	 */
	int linsert_before(const char* key, const char* pivot,
		const char* value);
	int linsert_before(const char* key, const char* pivot,
		size_t pivot_len, const char* value, size_t value_len);

	/**
	 * Append a new element after the specified element in the list.
	 * append a new element after the specified element in the list
	 * @param key {const char*} List object's key.
	 *  the key of the specified list
	 * @param pivot {const char*} A specified element in list.
	 *  the specified element
	 * @param value {const char*} New element value.
	 *  the new element
	 * @reutrn {int} Returns number of elements in list specified by key:
	 *  return the number of elements in the list specifed by the key:
	 *  -1 -- Indicates error occurred or key is not a list object.
	 *        error happened or it is not a list object specified by key
	 *  >0 -- Current list's element count.
	 *        the number of elements in list specified by the key
	 */
	int linsert_after(const char* key, const char* pivot,
		const char* value);
	int linsert_after(const char* key, const char* pivot,
		size_t pivot_len, const char* value, size_t value_len);

	/**
	 * Get the number of elements in list specified the given key.
	 * get the number of elements in list specified the given key
	 * @param key {const char*} List object's key.
	 *  the list's key
	 * @return {int} Returns length of specified list (element count). Returns -1 if error occurred.
	 *  return the number of elements in list, -1 if error
	 */
	int llen(const char* key);

	/**
	 * Remove and get the element in the list's head.
	 * remove and get the element in the list's head
	 * @param key {const char*} Element object's key.
	 *  the key of one list
	 * @param buf {string&} Store popped element value.
	 *  store the element when successful.
	 * @return {int} Return value meaning: >0 -- Indicates successfully got one element and return value indicates element length.
	 *  -1 -- Indicates error occurred, object is not a list specified
	 *  by the key, or list specified by key is empty.
	 *  return value as below:
	 *   >0: get one element successfully and return the length of element
	 *  -1: error happened, or the oject is not a list specified
	 *      by the key, or the list specified by key is empty
	 */
	int lpop(const char* key, string& buf);

	/**
	 * Add one or more element(s) to the head of a list.
	 * add one or more element(s) to the head of a list
	 * @param key {const char*} List object's key.
	 *  the list key
	 * @param first_value {const char*} First non-empty string. The last parameter of variable args must
	 *  be NULL.
	 *  the first no-NULL element of the variable args, the last arg must
	 *  be NULL indicating the end of the args.
	 * @return {int} Returns current list's element count after adding. Returns -1 to indicate error occurred or key
	 *  is not a list object. When key does not exist, creates a new list object and adds elements.
	 *  return the number of elements in list. -1 if error happened,
	 *  or the object specified by key is not a list.
	 */
	int lpush(const char* key, const char* first_value, ...);
	int lpush(const char* key, const char* values[], size_t argc);
	int lpush(const char* key, const std::vector<string>& values);
	int lpush(const char* key, const std::vector<const char*>& values);
	int lpush(const char* key, const char* values[], const size_t lens[],
		size_t argc);

	/**
	 * Add a new element before the head of a list, only if the list exists.
	 * add a new element before the head of a list, only if the list exists
	 * @param key {const char*} List object's key.
	 *  the list's key
	 * @param value {const char*} New element to be added to list.
	 *  the new element to be added
	 * @return {int} Returns current list's element count, as below:
	 *  return the number of elements in the list:
	 *  -1: Error occurred or key is not a list object.
	 *      error or the key isn't refer to a list
	 *   0: Key object does not exist.
	 *      the list specified by the given key doesn't exist
	 *  >0: Current list's element count after adding.
	 *      the number of elements in list specifed by key after added
	 */
	int lpushx(const char* key, const char* value);
	int lpushx(const char* key, const char* value, size_t len);

	/**
	 * Get a range of elements from list, the range is specified by
	 * start and end, and the start begins with 0, -1 means the end.
	 * get a range of elements from list, the range is specified by
	 * start and end, and the start begins with 0, -1 means the end
	 * @param key {const char*} List object's key.
	 *  the specified key of one list
	 * @param start {int} Start subscript value.
	 *  the start subscript of list
	 * @param end {int} End subscript value.
	 *  the end subscript of list
	 * @param result {std::vector<string>*} When not empty, stores element set in specified range.
	 *  if not NULL, result will be used to store the results
	 * @return {bool} Whether operation was successful. Returns false to indicate error occurred or key is not a list object.
	 *  if success for this operation, false if the key is not a list or
	 *  error happened
	 *  Examples:
	 *  for example:
	 *  1) When start = 0, end = 10, it specifies 11 elements from subscript 0 to 10.
	 *     if start is 0 and end is 10, then the subscript range is
	 *     between 0 and 10(include 10).
	 *  2) When start = -1, end = -2, it specifies from last element backward to second element (2 elements).
	 *     if start is -1 and end is -2, the range is from the end and
	 *     backward the second element.
	 *
	 *  When successful, result can be obtained by one of the following ways:
	 *  the result can be got by one of the ways as below:
	 *
	 *  1. Pass non-empty storage container address in function call.
	 *     the most easily way is to set a non-NULL result parameter
	 *     for this function
	 *  2. Get specified subscript element by base class function get_value.
	 *     get the specified subscript's element by redis_command::get_value 
	 *  3. Get specified subscript element object (redis_result) by base class function get_child, then get
	 *     element value through redis_result::argv_to_string.
	 *     get redis_result object with the given subscript, and get the
	 *     element by redis_result::argv_to_string
	 *  4. Get overall result object redis_result by base class function get_result, then get
	 *     first element object by redis_result::get_child, then get element value by method 2 above.
	 *     get redis_result object by redis_command::get_result, and get
	 *     the first element by redis_result::get_child, then get the
	 *     element by the way same as the way 2 above.
	 *  5. Get child array by base class function get_children, then get element value from each
	 *     redis_result object in array through redis_result's method argv_to_string.
	 *     get child array by redis_command::get_children, and get the
	 *     element from one of redis_result array by argv_to_string.
	 */
	bool lrange(const char* key, int start, int end,
		std::vector<string>* result);

	/**
	 * Remove the first count occurrences of elements equal to value
	 * from the list stored at key.
	 * remove the first count occurrences of elements equal to value
	 * from the list stored at key
	 * @param key {const char*} List object's key.
	 *  the key of a list
	 * @param count {int} Number of elements to remove. count meaning:
	 *  the first count of elements to be removed, as below:
	 *  count > 0 : Remove elements equal to value moving from head to tail, count is count.
	 *              remove elements equal to value moving from head to tail
	 *  count < 0 : Remove elements equal to value moving from tail to head, count is absolute value of count.
	 *              remove elements equal to value moving from tail to head
	 *  count = 0 : Remove all elements equal to value.
	 *              remove all elements equal to value
	 * @param value {const char*} Specified element value. Need to compare with values in list.
	 *  the specified value for removing elements
	 * @return {int} Number of elements removed. Return value meaning:
	 *  the count of elements removed, meaning show below:
	 *  -1: Error occurred or key is not a list object.
	 *      error happened or the key is not refer to a list
	 *   0: Key does not exist or number of elements removed is 0.
	 *      the key does not exist or the count of elements removed is 0
	 *  >0: Number of elements successfully removed.
	 *      the count of elements removed successfully
	 */
	int lrem(const char* key, int count, const char* value);
	int lrem(const char* key, int count, const char* value, size_t len);

	/**
	 * Set the value of a element in a list by its index, if the index
	 * out of bounds or the key of list not exist, an error will happen.
	 * set the value of a element in a list by its index, if the index
	 * out of bounds or the key of list not exist, an error will happen.
	 * @param key {const char*} List object's key.
	 *  the key of list
	 * @param idx {int} Index position. When negative, iterating data will be
	 *  from tail to head, or be from head to tail. For example: 0 means first element at head, -1 means first element from tail.
	 *  the index in the list, if it's negative, iterating data will be
	 *  from tail to head, or be from head to tail.
	 * @param value {const char*} Element new value.
	 *  the new value of the element by its index
	 * @return {bool} Returns false when key is not a list object, key does not exist, or idx is out of bounds.
	 *  if success. false if the object of the key isn't list, or key's
	 *  list not exist, or the index out of bounds.
	 */
	bool lset(const char* key, int idx, const char* value);
	bool lset(const char* key, int idx, const char* value, size_t len);

	/**
	 * Remove elements in a list by range betwwen start and end.
	 * remove elements in a list by range betwwen start and end.
	 * @param key {const char*} List object's key.
	 *  the key of a list
	 * @param start {int} Start subscript value.
	 *  the start index in a list
	 * @param end {int} End subscript value.
	 *  the end index in a list
	 * @return {bool} Whether operation was successful. Returns false to indicate error occurred, key's object is not
	 *  a list, or key's object does not exist. Returns true when successfully deleted or key object does not exist.
	 *  if success. false if error happened, or the key's object is not
	 *  a list, or the key's object not exist.
	 */
	bool ltrim(const char* key, int start, int end);

	/**
	 * Remove and get the last element of a list.
	 * remove and get the last element of a list
	 * @param key {const char*} Element object's key.
	 *  the key of the list
	 * @param buf {string&} Store popped element value.
	 *  store the element pop from list
	 * @return {int} Return value meaning: >0 -- Indicates successfully got one element and return value indicates element length.
	 *  -1 -- Indicates error occurred, object is not a list specified
	 *  by the key, or list specified by key is empty.
	 *  return value as below:
	 *   >0: get one element successfully and return the length of element
	 *  -1: error happened, or the oject is not a list specified
	 *      by the key, or the list specified by key is empty
	 */
	int rpop(const char* key, string& buf);

	/**
	 * Remove the last element in a list, prepend it to another list
	 * and return it.
	 * remove the last element in a list, prepend it to another list
	 * and return it.
	 * @param src {const char*} Source list's key.
	 *  the key of the source list
	 * @param dst {const char*} Destination list's key.
	 *  the key of the destination list
	 * @param buf {string*} When not empty, stores element popped from src tail.
	 *  if not NULL, it will store the element
	 * @return {bool} When successfully popped element from src list tail and pushed to dst list head,
	 *  this function returns true. Returns false to indicate error occurred or one of src/dst is not a list object.
	 *  true if the element was removed from a list to another list,
	 *  false if error happened, one of src or dst is not a list.
	 */
	bool rpoplpush(const char* src, const char* dst, string* buf = NULL);

	/**
	 * Append one or multiple values to a list.
	 * append one or multiple values to a list
	 * @param key {const char*} List object's key.
	 *  the key of a list
	 * @param first_value {const char*} First element of variable args must be not NULL, and the
	 *  last arg must be NULL indicating the end of the args.
	 *  the first element of a variable args must be not NULL, and the
	 *  last arg must be NULL indicating the end of the args.
	 * @return {int} Returns number of elements in list specified by key after adding. Returns -1 to indicate error
	 *  occurred, or key's object isn't a list. If the list by the key does not exist, a new list will be created with the key.
	 *  return the number of a list specified by a key. -1 if error
	 *  happened, or the key's object isn't a list, if the list by the
	 *  key doese not exist, a new list will be created with the key.
	 */
	int rpush(const char* key, const char* first_value, ...);
	int rpush(const char* key, const char* values[], size_t argc);
	int rpush(const char* key, const std::vector<string>& values);
	int rpush(const char* key, const std::vector<const char*>& values);
	int rpush(const char* key, const char* values[], const size_t lens[],
		size_t argc);

	/**
	 * Append one or multiple values to a list only if the list exists.
	 * append one or multiple values to a list only if the list exists.
	 * @param key {const char*} List object's key.
	 *  the key of a list
	 * @param value {const char*} New element to be added.
	 *  the new element to be added.
	 * @return {int} Returns current list's element count, as below:
	 *  return the number of the list, as below:
	 *  -1: Error occurred, or key's object isn't a list.
	 *      error happened, or the key's object isn't a list
	 *   0: Key object does not exist.
	 *      the key's object doesn't exist
	 *  >0: Current list's element count after adding.
	 *     the number of elements in the list after adding.
	 */
	int rpushx(const char* key, const char* value);
	int rpushx(const char* key, const char* value, size_t len);

private:
	int linsert(const char* key, const char* pos, const char* pivot,
		size_t pivot_len, const char* value, size_t value_len);
	int pushx(const char* cmd, const char* key,
		const char* value, size_t len);
	int pop(const char* cmd, const char* key, string& buf);
	bool bpop(const char* cmd, const std::vector<const char*>& keys,
		size_t timeout, std::pair<string, string>& result);
	bool bpop(const char* cmd, const std::vector<string>& keys,
		size_t timeout, std::pair<string, string>& result);
	bool bpop(std::pair<string, string>& result);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

