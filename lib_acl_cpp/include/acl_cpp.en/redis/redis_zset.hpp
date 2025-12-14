#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <map>
#include <utility>
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

class ACL_CPP_API redis_zset : virtual public redis_command {
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_zset();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_zset(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	redis_zset(redis_client_cluster* cluster);

	redis_zset(redis_client_pipeline* pipeline);

	ACL_CPP_DEPRECATED
	redis_zset(redis_client_cluster* cluster, size_t max_conns);

	virtual ~redis_zset();

	/////////////////////////////////////////////////////////////////////

	/**
	 * Add members to corresponding key sorted set.
	 * add one or more members to a sorted set, or update its score if
	 * it already exists
	 * @param key {const char*} Sorted set key value.
	 *  the key of a sorted set
	 * @param members "score-member" collection.
	 *  the set storing values and stores
	 * @return {int} Number of successfully added "score-member" pairs.
	 *  the number of elements added to the sorted set, not including
	 *  elements already existing for which the score was updated
	 * 0: Indicates nothing was added, possibly because member already exists and
	 * score was updated.
	 *     nothing was added to the sorted set
	 * -1: Indicates error occurred or key is not a sorted set object.
	 *     error or it was not a sorted set by the key
	 * >0: Number of members added.
	 *     the number of elements added
	 */
	int zadd(const char* key, const std::map<string, double>& members,
		const std::vector<string>* options = NULL);
	int zadd(const char* key,
		const std::vector<std::pair<string, double> >&members);
	int zadd(const char* key,
		const std::vector<std::pair<const char*, double> >&members);
	int zadd(const char* key, const std::vector<string>& members,
		const std::vector<double>& scores);
	int zadd(const char* key, const std::vector<const char*>& members,
		const std::vector<double>& scores);
	int zadd(const char* key, const char* members[], double scores[],
		size_t size);
	int zadd(const char* key, const char* members[], size_t members_len[],
		double scores[], size_t size);

	int zadd_with_ch_xx(const char* key, const std::map<string, double>& members);
	int zadd_with_ch_nx(const char* key, const std::map<string, double>& members);

	bool zadd_with_incr(const char* key, const char* member, size_t len,
		double score, double* result = NULL, const char* option = NULL);
	bool zadd_with_incr(const char* key, const char* member,
		double score, double* result = NULL, const char* option = NULL);
	bool zadd_with_incr_xx(const char* key, const char* member,
		double score, double* result = NULL);
	bool zadd_with_incr_nx(const char* key, const char* member,
		double score, double* result = NULL);

	/**
	 * Get number of members in corresponding sorted set.
	 * get the number of elements in a sorted set
	 * @param key {const char*} Sorted set key value.
	 *  the key of a a sorted set
	 * @return {int} Number of members in sorted set.
	 *  the number of elements of the sorted set
	 *   0: Key does not exist.
	 *      the key doesn't exist
	 * -1: Error occurred or data object corresponding to key is not a valid sorted
	 * set object.
	 *      error or it wasn't a sorted set by the key
	 *  >0: Number of members in data object corresponding to current key value.
	 *      the number of elements in the sorted set
	 */
	int zcard(const char* key);

	/**
	 * Get number of members in key sorted set with scores within specified range.
	 * get the number of elements in a sorted set with scores within
	 * the given values
	 * @param key {const char*} Sorted set key value.
	 *  the key of a sorted set
	 * @param min {double} Minimum score.
	 *  the min score specified
	 * @param max {double} Maximum score.
	 *  the max socre specified
	 * @return {int} Number of members in specified range.
	 *  the number of elements in specified score range
	 * 0: No members in specified score range corresponding to this key sorted set,
	 * or KEY sorted set's corresponding score range members are empty.
	 *     nothing in the specified score range, or the key doesn't exist
	 * -1: Error occurred or data object corresponding to key is not a valid sorted
	 * set object.
	 *     error or it is not a sorted set by the key
	 */
	int zcount(const char* key, double min, double max);

	/**
	 * Increment score of a certain member in key sorted set by inc.
	 * increase the score of a memeber in a sorted set
	 * @param key {const char*} Sorted set key value.
	 *  the key of the sorted set
	 * @param inc {double} Increment value.
	 *  the value to be increased
	 * @param member {const char*} Member name in sorted set.
	 *  the specified memeber of a sorted set
	 * @param result {double*} When not empty, stores score result.
	 *  if not null, it will store the score result after increment
	 * @return {bool} Whether operation was successful.
	 *  if successful about the operation
	 */
	bool zincrby(const char* key, double inc, const char* member,
		double* result = NULL);
	bool zincrby(const char* key, double inc, const char* member,
		size_t len, double* result = NULL);

	/**
	 * Get member list at specified position range in key sorted set. Members are
	 * sorted by score value in ascending order.
	 * get the specified range memebers of a sorted set sotred at key
	 * @param key {const char*} Sorted set key value.
	 *  the key of a sorted set
	 * @param start {int} Start subscript position.
	 *  the begin index of the sorted set
	 * @param stop {int} End subscript position (including this position).
	 *  the end index of the sorted set
	 * @param result {std::vector<string>*} When not empty, stores result.
	 * Internally first calls
	 *  result.clear() to clear all elements.
	 *  if not NULL, it will store the memebers result
	 * @return {int} Number of members in result.
	 *  the number of memebers
	 *  0: Indicates result is empty or key does not exist.
	 *     the result is empty or the key doesn't exist
	 * -1: Indicates error occurred or key is not a sorted set object.
	 *     error or it's not a sorted set by the key
	 * >0: Number of result members.
	 *     the number of the memebers result
	 * Note: About subscript position: 0 indicates first member, 1 indicates second
	 * member, -1 indicates last member.
	 *     -2 indicates second-to-last member, and so on.
	 *  Notice: about the index, element by index 0 is the first
	 *   of the sorted set, element by index -1 is the last one. 
	 *
	 *  When successful, result can be obtained by one of the following ways:
	 *  when success, the result can be got by one of the below proccess: 
	 *  1. Pass non-empty storage container address in function call.
	 *     the most easily way is to set a non-NULL result parameter
	 *     for this function
	 *  2. Get specified subscript element value by base class function get_value.
	 *     get the specified subscript's element by redis_command::get_value
	 * 3. Get specified subscript element object (redis_result) by base class
	 * function get_child, then get
	 *     element value through redis_result::argv_to_string.
	 *     redis_result::argv_to_string to get element value.
	 *     get redis_result object with the given subscript, and get the
	 *     element by redis_result::argv_to_string
	 * 4. Get overall result object redis_result by base class function get_result,
	 * then get
	 * first element object by redis_result::get_child, then get element value by
	 * method 2 above.
	 *     get redis_result object by redis_command::get_result, and get
	 *     the first element by redis_result::get_child, then get the
	 *     element by the way same as the way 2 above.
	 * 5. Get child array by base class function get_children, then get element
	 * value from each
	 * redis_result object in array through redis_result's method argv_to_string.
	 *     get child array by redis_command::get_children, and get the
	 *     element from one of redis_result array by argv_to_string
	 */
	int zrange(const char* key, int start, int stop,
		std::vector<string>* result);

	/**
	 * Get member-score list at specified position range in key sorted set. Members
	 * are sorted by score value in ascending order.
	 * @param key {const char*} Sorted set key value.
	 * @param start {int} Start subscript position.
	 * @param stop {int} End subscript position (including this position).
	 * @param out Stores "member-score" pair collection. Internally first calls
	 * out.clear().
	 * @return {int} Number of members in result.
	 *  0: Indicates result is empty or key does not exist.
	 * -1: Indicates error occurred or key is not a sorted set object.
	 * >0: Number of result members.
	 * Note: About subscript position: 0 indicates first member, 1 indicates second
	 * member, -1 indicates last member.
	 *     -2 indicates second-to-last member, and so on.
	 */
	int zrange_with_scores(const char* key, int start, int stop,
		std::vector<std::pair<string, double> >& out);

	/**
	 * In sorted set key, get members with score values between min and max
	 * (including min and max).
	 * Sorted set members are sorted by score value in ascending (small to large)
	 * order.
	 * @param key {const char*} Sorted set key value.
	 * @param min {double} Minimum score.
	 * @param max {double} Maximum score.
	 * @param out {std::vector<string>*} When not empty, stores member result
	 * collection.
	 * @param offset {const int*} When not empty, indicates start subscript of
	 * result collection.
	 * @param count {const int*} When not empty, indicates number of result members
	 * to get.
	 * @return {int} Number of members in result.
	 *  0: Indicates result is empty or key does not exist.
	 * -1: Indicates error occurred or key is not a sorted set object.
	 * >0: Number of result members.
	 *  Note: offset and count are effective only when both are non-empty pointers.
	 *
	 *  When successful, result can be obtained by one of the following ways:
	 *  1. Pass non-empty storage container address in function call.
	 *  2. Get specified subscript element value by base class function get_value.
	 * 3. Get specified subscript element object (redis_result) by base class
	 * function get_child, then get
	 *     element value through redis_result::argv_to_string.
	 * 4. Get overall result object redis_result by base class function get_result,
	 * then get
	 * first element object by redis_result::get_child, then get element value by
	 * method 2 above.
	 * 5. Get child array by base class function get_children, then get element
	 * value from each
	 * redis_result object in array through redis_result's method argv_to_string.
	 */
	int zrangebyscore(const char* key, double min, double max,
		std::vector<string>* out, const int* offset = NULL,
		const int* count = NULL);

	/**
	 * In sorted set key, get members with score values between min and max
	 * (including min and max).
	 * Sorted set members are sorted by score value in ascending (small to large)
	 * order.
	 * @param key {const char*} Sorted set key value.
	 * @param min {const char*} String representation of minimum score.
	 * @param max {const char*} String representation of maximum score.
	 * @param out {std::vector<string>*} When not empty, stores member result
	 * collection.
	 * @param offset {const int*} When not empty, indicates start subscript of
	 * result collection.
	 * @param count {const int*} When not empty, indicates number of result members
	 * to get.
	 * @return {int} Number of members in result.
	 *  0: Indicates result is empty or key does not exist.
	 * -1: Indicates error occurred or key is not a sorted set object.
	 * >0: Number of result members.
	 *  Note:
	 * 1. offset and count are effective only when both are non-empty pointers.
	 * 2. min and max can use -inf and +inf to represent infinity.
	 * 3. By default, range value uses closed interval (less than or equal to or
	 * greater than or equal to). You can also add
	 * opening parenthesis ( before value to use optional open interval (less than
	 * or greater than). For example:
	 * 3.1. "ZRANGEBYSCORE zset (1 5" means query members with 1 < score <= 5.
	 * 3.2. "ZRANGEBYSCORE zset (5 (10" means query members with 5 < score < 10.
	 *
	 *  When successful, result can be obtained by one of the following ways:
	 *  1. Pass non-empty storage container address in function call.
	 *  2. Get specified subscript element value by base class function get_value.
	 * 3. Get specified subscript element object (redis_result) by base class
	 * function get_child, then get
	 *     element value through redis_result::argv_to_string.
	 * 4. Get overall result object redis_result by base class function get_result,
	 * then get
	 * first element object by redis_result::get_child, then get element value by
	 * method 2 above.
	 * 5. Get child array by base class function get_children, then get element
	 * value from each
	 * redis_result object in array through redis_result's method argv_to_string.
	 */
	int zrangebyscore(const char* key, const char* min, const char* max,
		std::vector<string>* out, const int* offset = NULL,
		const int* count = NULL);

	/**
	 * In sorted set key, get member-score pairs with score values between min and
	 * max (including min and max).
	 * Sorted set members are sorted by score value in ascending (small to large)
	 * order. Values (min/max) use
	 * numeric representation.
	 * @param out Stores result. Internally first calls out.clear().
	 * @return {int} Number of members in result.
	 */
	int zrangebyscore_with_scores(const char* key, double min, double max,
		std::vector<std::pair<string, double> >& out,
		const int* offset = NULL, const int* count = NULL);

	/**
	 * In sorted set key, get member-score pairs with score values between min and
	 * max (including min and max).
	 * Sorted set members are sorted by score value in ascending (small to large)
	 * order. Values (min/max) use
	 * string representation.
	 * @param out Stores result. Internally first calls out.clear().
	 * @return {int} Number of members in result.
	 */
	int zrangebyscore_with_scores(const char* key, const char* min,
		const char* max, std::vector<std::pair<string, double> >& out,
		const int* offset = NULL, const int* count = NULL);

	/**
	 * Get rank (subscript starts from 0) of member member in sorted set key.
	 * Sorted set members are sorted by score
	 * value in ascending (small to large) order.
	 * @param key {const char*} Sorted set key value.
	 * @param member {const char*} Member name.
	 * @param len {size_t} Length of member.
	 * @return {int} Subscript position value. -1 -- Error occurred, key is not a
	 * sorted set object, or member does not exist.
	 */
	int zrank(const char* key, const char* member, size_t len);
	int zrank(const char* key, const char* member);

	/**
	 * Remove certain members from sorted set.
	 * @param key {const char*} Sorted set key value.
	 * @param first_member {const char*} First member in list of members to be
	 * removed.
	 * @return {int} Number of successfully removed members. -1 indicates error
	 * occurred or key is not a sorted set object.
	 * 0 indicates sorted set does not exist or member does not exist. > 0
	 * indicates number of successfully removed members.
	 */
	int zrem(const char* key, const char* first_member, ...);
	int zrem(const char* key, const std::vector<string>& members);
	int zrem(const char* key, const std::vector<const char*>& members);
	int zrem(const char* key, const char* members[], const size_t lens[],
		size_t argc);

	/**
	 * Remove members at specified rank (range) in sorted set key.
	 * You can directly use subscripts start and stop to specify range. start and
	 * stop are included,
	 * subscript range starts from 0, meaning 0 indicates first member in sorted
	 * set,
	 * 1 indicates second member in sorted set, and so on.
	 * You can also use negative subscripts, where -1 indicates last member, -2
	 * indicates second-to-last member, and so on.
	 * @param key {const char*} Sorted set key value.
	 * @param start {int} Start subscript position, starting from 0.
	 * @param stop {int} End subscript position.
	 * @return {int} Number of removed members.
	 * 0: Indicates key does not exist or no members were removed (range does not
	 * exist).
	 * -1: Indicates error occurred or key is not a sorted set object.
	 */
	int zremrangebyrank(const char* key, int start, int stop);

	/**
	 * Remove members with score values between min and max (including min and max)
	 * in sorted set key. Starting from version 2.1.6, members with score values
	 * equal to min and max can also be excluded. For details, see ZRANGEBYSCORE
	 * command.
	 * @param key {const char*} Sorted set key value.
	 * @param min {double} Minimum score.
	 * @param max {double} Maximum score.
	 * @return {int} Number of successfully removed members. -1 indicates error
	 * occurred or key is not a sorted set object.
	 * 0 indicates sorted set does not exist or member does not exist. > 0
	 * indicates number of successfully removed members.
	 */
	int zremrangebyscore(const char* key, double min, double max);

	/**
	 * Remove members with score values between min and max (including min and max)
	 * in sorted set key. Starting from version 2.1.6, members with score values
	 * equal to min and max can also be excluded. For details, see ZRANGEBYSCORE
	 * command.
	 * @param key {const char*} Sorted set key value.
	 * @param min {const char*} String format minimum score. For details, see
	 * zrangebyscore notes.
	 * @param max {const char*} String format maximum score.
	 * @return {int} Number of successfully removed members. -1 indicates error
	 * occurred or key is not a sorted set object.
	 * 0 indicates sorted set does not exist or member does not exist. > 0
	 * indicates number of successfully removed members.
	 */
	int zremrangebyscore(const char* key, const char* min, const char* max);

	/**
	 * Get member list at specified position range in key sorted set. Members are
	 * sorted by score value in descending order.
	 * @param key {const char*} Sorted set key value.
	 * @param start {int} Start subscript position.
	 * @param stop {int} End subscript position (including this position).
	 * @param result {std::vector<string>*} When not empty, stores result.
	 * Note: About subscript position: 0 indicates first member, 1 indicates second
	 * member, -1 indicates last member.
	 *     -2 indicates second-to-last member, and so on.
	 * @return {int} Number of result members. -1 indicates error.
	 */
	int zrevrange(const char* key, int start, int stop,
		std::vector<string>* result);

	/**
	 * Get member-score list at specified position range in key sorted set. Members
	 * are sorted by score value in descending order.
	 * @param key {const char*} Sorted set key value.
	 * @param start {int} Start subscript position.
	 * @param stop {int} End subscript position (including this position).
	 * @param out Stores "member-score" pair collection. Internally first calls
	 * out.clear().
	 * Note: About subscript position: 0 indicates first member, 1 indicates second
	 * member, -1 indicates last member.
	 *     -2 indicates second-to-last member, and so on.
	 * @return {int} Number of result members. -1 indicates error.
	 */
	int zrevrange_with_scores(const char* key, int start, int stop,
		std::vector<std::pair<string, double> >& out);

	/**
	 * In sorted set key, get members with score values between min and max
	 * (including min and max).
	 * Sorted set members are sorted by score value in descending (large to small)
	 * order.
	 * @param key {const char*} Sorted set key value.
	 * @param min {const char*} String representation of minimum score.
	 * @param max {const char*} String representation of maximum score.
	 * @param out {std::vector<string>*} When not empty, stores member result
	 * collection.
	 * @param offset {const int*} When not empty, indicates start subscript of
	 * result collection.
	 * @param count {const int*} When not empty, indicates number of result members
	 * to get.
	 * @return {int} Number of members in result.
	 *  0: Indicates result is empty or key does not exist.
	 * -1: Indicates error occurred or key is not a sorted set object.
	 * >0: Number of result members.
	 *  Note:
	 * 1. offset and count are effective only when both are non-empty pointers.
	 * 2. min and max can use -inf and +inf to represent infinity.
	 * 3. By default, range value uses closed interval (less than or equal to or
	 * greater than or equal to). You can also add
	 * opening parenthesis ( before value to use optional open interval (less than
	 * or greater than). For example:
	 * 3.1. "ZRANGEBYSCORE zset (1 5" means query members with 1 < score <= 5.
	 * 3.2. "ZRANGEBYSCORE zset (5 (10" means query members with 5 < score < 10.
	 */
	//int zrevrangebyscore(const char* key, const char* min, const char* max,
	//	std::vector<string>* out, const int* offset = NULL,
	//	const int* count = NULL);

	/**
	 * In sorted set key, get member-score pairs with score values between min and
	 * max (including min and max).
	 * Sorted set members are sorted by score value in descending (large to small)
	 * order. Values (min/max) use
	 * numeric representation.
	 * @param out Stores score-member pair result. Internally first calls
	 * out.clear().
	 * @param count {const int*} When not empty, indicates number of result members
	 * to get.
	 */
	int zrevrangebyscore_with_scores(const char* key, double min,
		double max, std::vector<std::pair<string, double> >& out,
		const int* offset = NULL, const int* count = NULL);
	int zrevrangebyscore_with_scores(const char* key, const char* min,
		const char* max, std::vector<std::pair<string, double> >& out,
		const int* offset = NULL, const int* count = NULL);

	/**
	 * Get rank (subscript starts from 0) of member member in sorted set key.
	 * Sorted set members are sorted by score
	 * value in descending (large to small) order.
	 * @param key {const char*} Sorted set key value.
	 * @param member {const char*} Member name.
	 * @param len {size_t} Length of member.
	 * @return {int} Subscript position value. -1 -- Error occurred, key is not a
	 * sorted set object, or member does not exist.
	 */
	int zrevrank(const char* key, const char* member, size_t len);
	int zrevrank(const char* key, const char* member);

	/**
	 * Get score value of member member in sorted set key.
	 * @param key {const char*} Sorted set key value.
	 * @param member {const char*} Member name.
	 * @param len {size_t} Length of member.
	 * @param result {double&} Stores score result.
	 * @return {bool} Returns false when member does not exist or error occurred.
	 * Otherwise returns true.
	 */
	bool zscore(const char* key, const char* member, size_t len,
		double& result);
	bool zscore(const char* key, const char* member, double& result);

	/**
	 * Compute union of one or more sorted sets. For given key sorted sets, compute
	 * union of numkeys specified
	 * sorted sets and store result (aggregation) in destination sorted set. By
	 * default, when a member's score
	 * value exists in multiple sorted sets, sum of scores of this member in all
	 * sorted sets is used as
	 * this member's score value in result.
	 * @param dst {const char*} Destination sorted set key value.
	 * @param keys Source sorted set key value-weight collection. When using weight
	 * option, you need to
	 * specify a multiplication factor for each sorted set respectively. Each
	 * sorted set's all members' score
	 * values need to be multiplied by this sorted set's factor before being passed
	 * to aggregation function.
	 *  If WEIGHTS option is not specified, multiplication factor defaults to 1.
	 * @param aggregate {const char*} Aggregation method. Default is SUM
	 * aggregation method. Aggregation methods are as follows:
	 * SUM: Sum of score values of a certain member in all sorted sets is used as
	 * this member's score value in result sorted set.
	 * MIN: Minimum score value of a certain member in all sorted sets is used as
	 * this member's score value in result sorted set.
	 * MAX: Maximum score value of a certain member in all sorted sets is used as
	 * this member's score value in result sorted set.
	 * @return {int} Number of elements (members) stored in destination sorted set
	 * result. When source
	 * sorted sets have duplicate members, only one member is stored. Returns -1 to
	 * indicate error.
	 */
	int zunionstore(const char* dst, const std::map<string, double>& keys,
		const char* aggregate = "SUM");

	int zunionstore(const char* dst, const std::vector<string>& keys,
		const std::vector<double>* weights = NULL,
		const char* aggregate = "SUM");

	/**
	 * Compute intersection of one or more sorted sets. For given key sorted sets,
	 * compute intersection of numkeys specified
	 * sorted sets and store result (aggregation) in destination sorted set. By
	 * default, when a member's score
	 * value exists in multiple sorted sets, sum of scores of this member in all
	 * sorted sets is used as
	 * this member's score value in result.
	 * @return {int} Number of elements (members) stored in destination sorted set
	 * result.
	 */
	int zinterstore(const char* dst, const std::map<string, double>& keys,
		const char* aggregate = "SUM");

	int zinterstore(const char* dst, const std::vector<string>& keys,
		const std::vector<double>* weights = NULL,
		const char* aggregate = "SUM");
	
	/**
	 * Incrementally iterate elements in sorted set, including element members and
	 * element scores.
	 * @param cursor {int} Cursor value. Write 0 when starting iteration.
	 * @param out Stores result. Internally uses append method. Cursor iterates and
	 * appends results to collection.
	 * To prevent results from growing too large, users should clear this parameter
	 * before each call.
	 * @param pattern {const char*} Matching pattern with glob style, effective
	 * when not empty.
	 * @param count {const size_t*} Limit maximum number of results. Effective only
	 * when pointer is not empty.
	 * @return {int} Next cursor position, as below:
	 *   0: Iteration finished.
	 *  -1: Error occurred.
	 * >0: Next cursor position. Regardless of how many results are obtained, you
	 * need to check out, as it may be empty.
	 */
	int zscan(const char* key, int cursor,
		std::vector<std::pair<string, double> >& out,
		const char* pattern = NULL, const size_t* count = NULL);

	/**
	 * When all members in sorted set have the same score, sorted set elements will
	 * be sorted by member name value
	 * lexicographical ordering (dictionary order). This function queries members
	 * in sorted set key
	 * with values between min and max.
	 * @param min {const char*} Minimum value string.
	 * @param max {const char*} Maximum value string.
	 * @param out {std::vector<string>*} When not empty, stores result.
	 * @param offset {const int*} When not empty, effective. Start subscript value
	 * for selecting from result.
	 * @param count {const int*} When not empty, effective. Number of elements to
	 * select from result at specified subscript position.
	 * @return {int} Number of members in result.
	 *  0: Indicates result is empty or key does not exist.
	 * -1: Indicates error occurred or key is not a sorted set object.
	 * >0: Number of result members.
	 * Note: About range selection:
	 * 1. Legal min and max strings include opening parenthesis ( and closing
	 * bracket [. Opening parenthesis ( indicates open interval (specified value
	 * will not be included in range), and [ indicates closed interval (specified
	 * value will be included in range).
	 * 2. Special values + and - can be used in min string and max string. + means
	 * positive infinity, and - means negative infinity. Therefore, when all
	 * members in a sorted set have the same score, sorted set
	 *   ZRANGEBYLEX <zset> - + command will return all elements in sorted set.
	 */
	int zrangebylex(const char* key, const char* min, const char* max,
		std::vector<string>* out, const int* offset = NULL,
		const int* count = NULL);

	/**
	 * When all members in a sorted set have the same score, for sorted set key,
	 * this function returns number of
	 * elements (members) with values between min and max in this set.
	 * @return {int} Number of elements in result.
	 */
	int zlexcount(const char* key, const char* min, const char* max);

	/**
	 * When all members in a sorted set have the same score, for sorted set key,
	 * this function removes all
	 * elements (members) with values between min and max in this set.
	 * @return {int} Number of removed elements.
	 */
	int zremrangebylex(const char* key, const char* min, const char* max);

	int zpopmin(const char* key,
		std::vector<std::pair<string, double> >& out, size_t count = 1);
	int zpopmax(const char* key,
		std::vector<std::pair<string, double> >& out, size_t count = 1);
	int bzpopmin(const char* key, size_t timeout, string& member,
		double* score = NULL);
	int bzpopmax(const char* key, size_t timeout, string& member,
		double* score = NULL);
	int bzpopmin(const std::vector<string>& keys, size_t timeout,
		string& member, double* score = NULL);
	int bzpopmax(const std::vector<string>& keys, size_t timeout,
		string& member, double* score = NULL);

private:
	int zrange_get(const char* cmd, const char* key, int start,
		int stop, std::vector<string>* result);
	int zrange_get_with_scores(const char* cmd, const char* key, int start,
		int stop, std::vector<std::pair<string, double> >& out);
	int zrangebyscore_get(const char* cmd, const char* key,
		const char* min, const char* max, std::vector<string>* out,
		const int* offset = NULL, const int* count = NULL);
	int zrangebyscore_get_with_scores(const char* cmd,
		const char* key, const char* min, const char* max,
		std::vector<std::pair<string, double> >& out,
		const int* offset = NULL, const int* count = NULL);
	int zstore(const char* cmd, const char* dst,
		const std::map<string, double>& keys, const char* aggregate);
	int zstore(const char* cmd, const char* dst, const std::vector<string>& keys,
		const std::vector<double>* weights, const char* aggregate);
	int zpop(const char* cmd, const char* key,
		std::vector<std::pair<string, double> >& out, size_t count);
	int get_with_scores(std::vector<std::pair<string, double> >& out);
	int bzpop(const char* cmd, const char* key, size_t timeout,
		string& member, double* score);
	int bzpop(const char* cmd, const std::vector<string>& keys,
		size_t timeout, string& member, double* score);
	int bzpop_result(string& member, double* score);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

