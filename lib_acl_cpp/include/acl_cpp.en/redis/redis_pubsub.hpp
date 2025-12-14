#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <map>
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

class ACL_CPP_API redis_pubsub : virtual public redis_command {
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_pubsub();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_pubsub(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	redis_pubsub(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_pubsub(redis_client_cluster* cluster, size_t max_conns);

	redis_pubsub(redis_client_pipeline* pipeline);

	virtual ~redis_pubsub();

	/////////////////////////////////////////////////////////////////////

	/**
	 * Send message to specified channel
	 * post a message to a channel
	 * @param channel {const char*} Target channel of sent message
	 *  the specified channel
	 * @param msg {const char*} Message content
	 *  the message to be sent
	 * @param len {size_t} Message length
	 *  the message's length
	 * @return {int} Number of subscribers that successfully received the message
	 *  the number of clients that received the message
	 *  -1: Indicates error
	 *      error happened
	 *   0: No subscribers
	 *      no client subscribe the channel
	 *  >0: Number of subscribers subscribed to this channel
	 *      the number of clients that received the message
	 */
	int publish(const char* channel, const char* msg, size_t len);

	/**
	 * Subscribe to messages from one or more given channels. After calling this function, operations can only send commands:
	 * subscribe, unsubscribe, psubscribe, punsubscribe, get_message. Only
	 * after unsubscribing from all channels (or connection reestablished) can this restriction be lifted
	 * subscribe one or more channel(s). Once the client enters the
	 * subscribed state it is not supposed to issue any other commands,
	 * except for additional SUBSCRIBE, PSUBSCRIBE, UNSUBSCRIBE
	 * and PUNSUBSCRIBE commands
	 * @param first_channel {const char*} First non-empty string channel in subscribed channel list.
	 *  For variable argument list, the last one must be NULL
	 *  the first non-NULL channel in the channel list, and the last
	 *  parameter must be NULL indicating the end of the channel list
	 * @return {int} Returns number of channels currently successfully subscribed (i.e., total number of all subscribed channels)
	 *  the number of channels subscribed by the current client
	 */
	int subscribe(const char* first_channel, ...);
	int subscribe(const std::vector<const char*>& channels);
	int subscribe(const std::vector<string>& channels);

	/**
	 * Unsubscribe from messages from one or more given channels
	 * stop listening for messages posted to the given channels
	 * @param first_channel {const char*} First channel in list of channels to unsubscribe
	 *  the fist channel in channel list, and the last parameter must be
	 *  NULL indicating the end of the channel list
	 * @return {int} Returns remaining number of subscribed channels
	 *  the rest channels listened by the current client
	 */
	int unsubscribe(const char* first_channel, ...);
	int unsubscribe(const std::vector<const char*>& channels);
	int unsubscribe(const std::vector<string>& channels);

	/**
	* Subscribe to one or more channels matching given patterns. Each pattern uses * as wildcard. After calling this function, operations
	* can only send commands: subscribe, unsubscribe, psubscribe, punsubscribe,
	* get_message. Only after unsubscribing from all channels (or connection reestablished) can this restriction be lifted
	* listen for messages published to channels matching the give patterns
	 * @param first_pattern {const char*} First matching pattern string
	 *  the first pattern in pattern list, the last parameter must be NULL
	 *  int the variable args
	 * @return {int} Returns number of channels currently successfully subscribed (i.e., total number of all subscribed channels)
	 *  the number of channels listened by the current client
	 */
	int psubscribe(const char* first_pattern, ...);
	int psubscribe(const std::vector<const char*>& patterns);
	int psubscribe(const std::vector<string>& patterns);

	/**
	 * Unsubscribe from messages from one or more channels based on pattern matching string
	 * stop listening for messaged posted to channels matching
	 * the given patterns
	 * @param first_pattern {const char*} First matching pattern string
	 *  the first parttern in a variable args ending with NULL
	 * @return {int} Returns remaining number of subscribed channels
	 *  the rest number of channels be listened by the client
	 */
	int punsubscribe(const char* first_pattern, ...);
	int punsubscribe(const std::vector<const char*>& patterns);
	int punsubscribe(const std::vector<string>& patterns);

	/**
	 * After subscribing to channels, can call this function in a loop to get subscription messages from subscribed channels.
	 * Can call this function to get messages from subscribed channels only after calling subscribe or psubscribe
	 * get messages posted to channels after SUBSCRIBE or PSUBSCRIBE
	 * @param channel {string&} Store channel name that currently has message
	 *  buffer for storing the channel associate with the msg
	 * @param msg {string&} Store currently obtained message content
	 *  store the message posted to the channel
	 * @param message_type {string*} will store messsage or pmessage
	 * @param pattern {string*} will store pattern set by psubscribe
	 * @param timeout {int} When this value >= 0, indicates wait timeout for messages (seconds)
     *  when timeout >= 0, which was used as the waiting time for reading(second)
	 * @return {bool} Whether successful. If returns false, it indicates error or timeout
	 *  true on success, false on error or waiting timeout
	 */
	bool get_message(string& channel, string& msg, string* message_type = NULL,
		string* pattern = NULL, int timeout = -1);

	/**
	 * List currently active channels: Active channels refer to those with at least one subscriber. Clients subscribed to patterns
	 * are not counted
	 * Lists the currently active channels.
	 * @param channels {std::vector<string>*} When not empty, stores channel result set
	 *  store the active channels
	 * @param first_pattern {const char*} First matching string as additional matching pattern,
	 *  this pointer can be NULL, in which case gets all active channels. For variable arguments, last parameter must be NULL
	 *  the first pattern in a variable args ending with NULL arg, and
	 *  the first arg can be NULL.
	 * @return {int} Returns number of active channels; -1 indicates error
	 *  the number of active channels. -1 if error
	 *
	 *  After successful operation, can get data through any of the following methods:
	 *  1. Base class method get_value to get element data at specified index
	 *  2. Base class method get_child to get element object (redis_result) at specified index, then get element data through
	 *     redis_result::argv_to_string method
	 *  3. Base class method get_result to get total result set object redis_result, then get an element object through
	 *     redis_result::get_child, then get element data through method specified in method 2
	 *  4. Base class method get_children to get result element array object, then get element data from each element object through methods in
	 *     redis_result through argv_to_string
	 *  5. Pass non-empty address of storage result object in calling method
	 *
	 */
	int pubsub_channels(std::vector<string>* channels,
		const char* first_pattern, ...);
	int pubsub_channels(const std::vector<const char*>& patterns,
		std::vector<string>* channels);
	int pubsub_channels(const std::vector<string>& patterns,
		std::vector<string>* channels);

	/**
	 * Return number of subscribers for given channels. Clients subscribed to patterns are not counted
	 * Returns the number of subscribers (not counting clients
	 * subscribed to patterns) for the specified channels.
	 * @param out {std::map<string, int>&} Store query result, where out->first stores
	 *  channel name, out->second stores number of subscribers for this channel
	 *  store the results
	 * @param first_channel {const char*} First channel
	 *  This pointer can be NULL, in which case gets all active channels. For variable arguments, last parameter must be NULL
	 *  the first pattern in a variable args ending with NULL arg, and
	 *  the first arg can be NULL.
	 * @return {int} Number of channels, -1 indicates error
	 */
	int pubsub_numsub(std::map<string, int>& out,
		const char* first_channel, ...);
	int pubsub_numsub(const std::vector<const char*>& channels,
		std::map<string, int>& out);
	int pubsub_numsub(const std::vector<string>& channels,
		std::map<string, int>& out);

	/**
	 * Return number of subscribed patterns. This command returns not the number of clients subscribed to patterns, but the total
	 * number of all patterns subscribed by clients
	 * Returns the number of subscriptions to patterns.
	 * @return {int} Total of all subscription patterns by clients, -1 indicates error
	 *  the number of patterns all the clients are subscribed to,
	 *  -1 if error.
	 */
	int pubsub_numpat();

private:
	int subop(const char* cmd, const std::vector<const char*>& channels);
	int subop_result(const char* cmd, const std::vector<const char*>& channels);
	int subop(const char* cmd, const std::vector<string>& channels);
	int subop_result(const char* cmd, const std::vector<string>& channels);
	int check_channel(const redis_result* obj, const char* cmd,
		const char* channel);
	int pubsub_numsub(std::map<string, int>& out);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

