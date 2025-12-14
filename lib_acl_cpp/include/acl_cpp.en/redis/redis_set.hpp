#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "../stdlib/string.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

class ACL_CPP_API redis_set : virtual public redis_command {
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_set();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	explicit redis_set(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	explicit redis_set(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_set(redis_client_cluster* cluster, size_t max_conns);

	explicit redis_set(redis_client_pipeline* pipeline);

	virtual ~redis_set();

	/////////////////////////////////////////////////////////////////////

	/**
	 * Add one or more member elements to the set stored at key. If member element
	 * already exists in the set, it will be ignored;
	 * 1) If key does not exist, create a new set containing only member elements.
	 * 2) When key is not a set type, an error will occur.
	 * add one or more members to a set stored at a key
	 * 1) if the key doesn't exist, a new set by the key will be created,
	 *    and add the members to the set
	 * 2) if the key exists and not a set's key, then error happened
	 * @param key {const char*} Set object key.
	 *  the key of a set
	 * @param first_member {const char*} First non-NULL member.
	 *  the first member of a variable args which isn't NULL, the last
	 *  arg of the args must be NULL indicating the end of args
	 * @return {int} Number of elements added to the set, not including
	 *  elements already present in the set.
	 *  the number of elements that were added to the set, not including
	 *  all the elements already present into the set. -1 if error
	 *  happened or it isn't a set stored by the key.
	 */
	int sadd(const char* key, const char* first_member, ...);
	int sadd(const char* key, const std::vector<const char*>& memsbers);
	int sadd(const char* key, const std::vector<string>& members);
	int sadd(const char* key, const char* argv[], size_t argc);
	int sadd(const char* key, const char* argv[], const size_t lens[],
		size_t argc);

	/**
	 * Remove and get one member from the set.
	 * remove and get one member from the set
	 * @param key {const char*} Set object key.
	 *  the key of the set
	 * @param buf {string&} Store the removed member.
	 *  store the member removed from the set
	 * @return {bool} Returns false when key does not exist or key is empty.
	 *  true if one member has been removed and got, false if the key
	 *  doesn't exist or it isn't a set stored at the key.
	 */
	bool spop(const char* key, string& buf);

	/**
	 * Get the number of members in the set.
	 * get the number of members in a set stored at the key
	 * @param key {const char*} Set object key.
	 *  the key of the set
	 * @return {int} Returns the number of members in this set object, as follows:
	 *  return int value as below:
	 *  -1: Error or it's not a set object.
	 *      error or it's not a set by the key
	 *   0: Member count is 0 or key does not exist.
	 *      the set is empty or the key doesn't exist
	 *  >0: Member count is non-zero.
	 *      the number of members in the set
	 */
	int scard(const char* key);

	/**
	 * Get all members in the set key.
	 * get all the members in a set stored at a key
	 * @param key {const char*} Set object key value.
	 *  the key of the set
	 * @param members {std::vector<string>*} When not empty, stores the result.
	 *  if not NULL, it will store the members.
	 * @return {int} Number of elements obtained, -1 indicates error or this key is
	 * not a set object.
	 *  the number of elements got, -1 if error happened or it't not
	 *  a set by the key.
	 *
	 *  If successful, one of the following methods can be used to get the result:
	 *  if successul, one of below ways can be used to get the result:
	 *  1. Pass a non-empty storage buffer address in the call.
	 *     the most easily way is to set a non-NULL result parameter
	 *     for this function 
	 *  2. Call base class method get_value with specified subscript element.
	 *     call redis_command::result_value with the specified subscript
	 * 3. Call base class method get_child with specified subscript element object
	 * (redis_result), then get
	 *     element string through redis_result::argv_to_string.
	 *     get redis_result object with the given subscript, and get the
	 *     element by redis_result::argv_to_string
	 * 4. Call base class method get_result to get the overall result object
	 * redis_result, then get
	 *     one element object through redis_result::get_child, then get the
	 *     element by the way same as the way 2 above.
	 *     get redis_result object by redis_command::get_result, and get
	 *     the first element by redis_result::get_child, then get the
	 *     element by the way same as the way 2 above
	 * 5. Call base class method get_children to get result element array, then get
	 * each
	 * element object through redis_result's method argv_to_string to serialize
	 * element string.
	 *     get child array by redis_command::get_children, and get the
	 *     element from one of redis_result array by argv_to_string
	 */
	int smembers(const char* key, std::vector<string>* members);

	/**
	 * Move member element from src set to dst set.
	 * move a member from one set to another
	 * @param src {const char*} Source set object key value.
	 *  the source key of a set
	 * @param dst {const char*} Target set object key value.
	 *  the destination key of a set
	 * @param member {const char*} Member in source set.
	 *  the member in the source set
	 * @return {int} Return value is as follows:
	 *  return int value as below:
	 *  -1: Error, or one of source/destination is not a set object.
	 *      error happened, or one of source and destination isn't a set
	 *   0: Source set does not exist or member is not in source set.
	 *     the source set or the member doesn't exist
	 *   1: Successfully moved one member from source set to
	 *      the destination set.
	 *      move successfully the member from source set to
	 *      the destination set
	 */
	int smove(const char* src, const char* dst, const char* member);
	int smove(const char* src, const char* dst, const string& member);
	int smove(const char* src, const char* dst,
		const char* member, size_t len);

	/**
	 * Return the members of the set resulting from the difference between the
	 * first set and all successive sets.
	 * return the members of the set resulting from the difference
	 * between the first set and all the successive sets.
	 * @param members {std::vector<string>*} When not empty, stores the result.
	 *  if not NULL, it will store the members.
	 * @param first_key {const char*} First non-empty set object key.
	 *  the key of the first set in a variable sets list, the last one
	 *  must be NULL indicating the end of the sets list.
	 * @return {int} Number of elements obtained, -1 indicates error or one key is
	 * not a set object.
	 *  the number of elements got, -1 if error happened or it't not
	 *  a set by the key.
	 *  If successful, one of the following methods can be used to get the result:
	 *  if successul, one of below ways can be used to get the result:
	 *  1. Pass a non-empty storage buffer address in the call.
	 *     the most easily way is to set a non-NULL result parameter
	 *     for this function
	 *  2. Call base class method get_value with specified subscript element.
	 *     get the specified subscript's element by redis_command::get_value
	 * 3. Call base class method get_child with specified subscript element object
	 * (redis_result), then get
	 *     element string through redis_result::argv_to_string.
	 *     get redis_result object with the given subscript, and get the
	 *     element by redis_result::argv_to_string
	 * 4. Call base class method get_result to get the overall result object
	 * redis_result, then get
	 *     one element object through redis_result::get_child, then get the
	 *     element by the way same as the way 2 above.
	 *     get redis_result object by redis_command::get_result, and get
	 *     the first element by redis_result::get_child, then get the
	 *     element by the way same as the way 2 above.
	 * 5. Call base class method get_children to get result element array, then get
	 * each
	 * element object through redis_result's method argv_to_string to serialize
	 * element string.
	 *     get child array by redis_command::get_children, and get the
	 *     element from one of redis_result array by argv_to_string
	 */
	int sdiff(std::vector<string>* members, const char* first_key, ...);
	int sdiff(const std::vector<const char*>& keys,
		std::vector<string>* members);
	int sdiff(const std::vector<string>& keys, std::vector<string>* members);

	/**
	 * Return the members of a set resulting from the intersection of all given
	 * sets.
	 * return the members of a set resulting from the intersection of
	 * all the give sets.
	 * @param members {std::vector<string>*} When not empty, stores the result.
	 *  if not NULL, it will store the result
	 * @param first_key {const char*} First set object key, non-NULL.
	 *  the key of the first set in a variable set list, which isn't NULL,
	 *  the last one must be NULL in the set list.
	 * @return {int} Returns the number of members, -1 indicates error or one key
	 * is not a set object.
	 *  return the number of the members, -1 if error happened or
	 *  it't not a set by the key.
	 */
	int sinter(std::vector<string>* members, const char* first_key, ...);
	int sinter(const std::vector<const char*>& keys, std::vector<string>* members);
	int sinter(const std::vector<string>& keys, std::vector<string>* members);

	/**
	 * Return the members of a set resulting from the union of all given sets.
	 * return the members of a set resulting from the union of all the
	 * given sets.
	 * @param members {std::vector<string>*} When not empty, stores the result.
	 *  if not NULL, it will store the result
	 * @param first_key {const char*} First set object key, non-NULL.
	 *  the key of the first set in a variable set list, which isn't NULL,
	 *  and the last arg must be NULL indicating the end of the set list.
	 * @return {int} Returns the number of members, -1 indicates error or one key
	 * is not a set object.
	 *  return the number of members, -1 if error happened or it's not
	 *  a set by the key.
	 */
	int sunion(std::vector<string>* members, const char* first_key, ...);
	int sunion(const std::vector<const char*>& keys, std::vector<string>* members);
	int sunion(const std::vector<string>& keys, std::vector<string>* members);

	/**
	 * This command is equal to SDIFF, but instead of returning the result, it
	 * stores the result in dst set.
	 * This command is equal to SDIFF, but instead of returning
	 * the resulting set, it is stored in destination.
	 * @param dst {const char*} Target set object key value.
	 *  the key of the destination set
	 * @param first_key {const char*} First non-empty set object key value.
	 *  the key of the first set in a variable set list, which isn't NULL,
	 *  and the last arg must be NULL indicating the end of the set list. 
	 * @return {int} Number of members in the result.
	 *  return the number of members, -1 if error happened or it's not
	 *  a set by the key.
	 */
	int sdiffstore(const char* dst, const char* first_key, ...);
	int sdiffstore(const char* dst, const std::vector<const char*>& keys);
	int sdiffstore(const char* dst, const std::vector<string>& keys);

	/**
	 * This command is equal to SINTER command, but instead of returning the
	 * result, it stores the result in dst set.
	 * This command is equal to SINTER, but instead of returning
	 * the resulting set, it is stored in destination.
	 * @param dst {const char*} Target set object key value.
	 *  the key of the destination set
	 * @param first_key {const char*} First non-empty set object key value.
	 *  the key of the first set in a variable set list, which isn't NULL,
	 *  and the last arg must be NULL indicating the end of the set list.
	 * @return {int} Number of members in the result.
	 *  return the number of members, -1 if error happened or it's not
	 *  a set by the key.
	 */
	int sinterstore(const char* dst, const char* first_key, ...);
	int sinterstore(const char* dst, const std::vector<const char*>& keys);
	int sinterstore(const char* dst, const std::vector<string>& keys);

	/**
	 * This command is equal to SUNION command, but instead of returning the
	 * result, it stores the result in dst set.
	 * This command is equal to SUNION, but instead of returning
	 * the resulting set, it is stored in destination.
	 * @param dst {const char*} Target set object key value.
	 *  the key of the destination set
	 * @param first_key {const char*} First non-empty set object key value.
	 *  the key of the first set in a variable set list, which isn't NULL,
	 *  and the last arg must be NULL indicating the end of the set list.
	 * @return {int} Number of members in the result.
	 *  return the number of members, -1 if error happened or it's not
	 *  a set by the key.
	 */
	int sunionstore(const char* dst, const char* first_key, ...);
	int sunionstore(const char* dst, const std::vector<const char*>& keys);
	int sunionstore(const char* dst, const std::vector<string>& keys);

	/**
	 * Determine if member element is a member of set key.
	 * determine if a given value is a member of a set
	 * @param key {const char*} Set object key value.
	 *  the key of a set
	 * @param member {const char*} Given value.
	 *  the given value
	 * @return {bool} Returns true to indicate yes, otherwise returns false (not a
	 * member or error or key
	 *  is not a set object).
	 *  true if the given is a member of the set, false if it's not a
	 *  member of the set, or error, or it's not a set by the key.
	 */
	bool sismember(const char* key, const char* member);
	bool sismember(const char* key, const char* member, size_t len);

	/**
	 * When only key is provided, returns one random element from the set. If the
	 * number of elements is also specified,
	 * returns an array of random elements with the specified count.
	 * get one or multiple memebers from a set
	 * @param key {const char*} Set object key value.
	 *  the key of a set
	 * @param out Store the result.
	 *  store the result
	 * @return {int} Number of members, -1 indicates error, 0 indicates no members.
	 *  the number of members, 0 if the set by the key is empty,
	 *  -1 if error happened.
	 */
	int srandmember(const char* key, string& out);
	int srandmember(const char* key, size_t n, std::vector<string>& out);

	/**
	 * Remove one or more member elements from the set stored at key. Non-existent
	 * member elements will be ignored.
	 * Remove the specified members from the set stored at key. if the
	 * member doesn't exist, it will be ignored.
	 * @param key {const char*} Set object key value.
	 *  the key of the set
	 * @param first_member {const char*} First non-NULL member in the list of
	 * members to be removed.
	 *  In variable parameter calls, the last parameter must be NULL.
	 *  the first non-NULL member to be removed in a variable member list,
	 *  and the last one must be NULL indicating the end of the list.
	 * @retur {int} Number of member elements removed. Returns -1 when error or
	 * it's not a set object. Returns 0 when key
	 *  does not exist or member does not exist.
	 *  the number of members be removed, 0 if the set is empty or the
	 *  key doesn't exist, -1 if error happened or it's not a set by key
	 */
	int srem(const char* key, const char* first_member, ...);
	int srem(const char* key, const std::vector<string>& members);
	int srem(const char* key, const std::vector<const char*>& members);
	int srem(const char* key, const char* members[], size_t lens[], size_t argc);

	/**
	 * Scan the members in the set stored at key.
	 * scan the members in a set stored at key
	 * @param key {const char*} Set key value.
	 *  the key of a set
	 * @param cursor {int} Cursor value, write 0 at the beginning.
	 *  the cursor value, which is 0 at begin
	 * @param out {std::vector<string>&} Store result. Internally uses append mode
	 * to add
	 * results to the result set. To prevent the overall result from being too
	 * large, users should clear
	 *  the result set before calling this method.
	 *  store result in appending mode.
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
	int sscan(const char* key, int cursor, std::vector<string>& out,
		const char* pattern = NULL, const size_t* count = NULL);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

