#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "../stdlib/string.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

class ACL_CPP_API redis_hyperloglog : virtual public redis_command {
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_hyperloglog();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_hyperloglog(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	redis_hyperloglog(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_hyperloglog(redis_client_cluster* cluster, size_t max_conns);

	redis_hyperloglog(redis_client_pipeline* pipeline);

	virtual ~redis_hyperloglog();

	/**
	 * Add any number of elements to the specified HyperLogLog
	 * add the specified elements to the specified HyperLogLog
	 * @param key {const char*} Specified key value
	 *  the key
	 * @param first_element {const char*} First element value in the element set, non-empty string
	 *  the first element of the elements list, and the last must be NULL
	 *  in the elements list
	 * @return {int} Whether the operation was successful, and indicates whether changes occurred. Return value meanings:
	 *  return the follow values:
	 *  1: Operation successful, and data was changed (new data added or old data changed)
	 *     successful, and the data was varied
	 *  0: Old data was modified but unchanged
	 *     nothing was changed after modifying the old data
	 * -1: Error or the corresponding key object is not a hyperloglog object
	 *     error or the keh isn't a hyperloglog type
	 */
	int pfadd(const char* key, const char* first_element, ...);
	int pfadd(const char* key, const std::vector<const char*>& elements);
	int pfadd(const char* key, const std::vector<string>& elements);

	/**
	 * Get the approximate number of deduplicated elements in the given key list's HyperLogLog
	 * return the approximated cardinality of the set(s) observed by
	 * the hyperloglog at key(s)
	 * @param first_key {const char*} First key in the key set, non-empty string
	 *  the firs key which must not be NULL of the keys list, and the
	 *  last parameter must be NULL in keys' list
	 * @return {int} Approximate number of deduplicated elements in the key list set
	 */
	int pfcount(const char* first_key, ...);
	int pfcount(const std::vector<const char*>& keys);
	int pfcount(const std::vector<string>& keys);

	/**
	 * Merge multiple HyperLogLogs into one HyperLogLog. The cardinality of the merged
	 * HyperLogLog is approximately the union of the visible sets of all input HyperLogLogs
	 * merge multiple different hyperloglogs into a single one
	 * @param dst {const char*} Key value of the destination HyperLogLog object
	 *  the single key as the destination
	 * @param first_src {const char*} Key of the first source HyperLogLog object in the source object set
	 *  the first source key which must not be NULL in the sources list,
	 *  and the last one must be NULL showing the end of the list
	 * @return {bool} Whether the operation was successful. Returns false if error or destination/source objects are not
	 *  HyperLogLog objects
	 *  true on success, false if the error or the dest/src are not
	 *  hyperloglog
	 */
	bool pfmerge(const char* dst, const char* first_src, ...);
	bool pfmerge(const char* dst, const std::vector<const char*>& keys);
	bool pfmerge(const char* dst, const std::vector<string>& keys);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

