#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

class redis_client;
class redis_client_cluster;

struct redis_stream_field
{
	string name;
	string value;
};

struct redis_stream_message
{
	string id;
	std::vector<redis_stream_field> fields;
};

struct redis_stream_messages
{
	string key;
	std::vector<redis_stream_message> messages;

	bool empty(void) const
	{
		return messages.empty();
	}

	size_t size(void) const
	{
		return messages.size();
	}
};

struct redis_xinfo_consumer
{
	string name;
	size_t pending;
	size_t idle;

	redis_xinfo_consumer(void)
	{
		pending = 0;
		idle    = 0;
	}
};

struct redis_xinfo_group
{
	string name;
	string last_delivered_id;
	size_t consumers;
	size_t pending;

	redis_xinfo_group(void)
	{
		consumers = 0;
		pending   = 0;
	}
};

struct redis_stream_info
{
	size_t length;
	size_t radix_tree_keys;
	size_t radix_tree_nodes;
	size_t groups;
	string last_generated_id;
	redis_stream_message first_entry;
	redis_stream_message last_entry;

	redis_stream_info(void)
	{
		length           = 0;
		radix_tree_keys  = 0;
		radix_tree_nodes = 0;
		groups           = 0;
	}
};

struct redis_pending_consumer
{
	string name;
	size_t pending_number;

	redis_pending_consumer(void)
	{
		pending_number = 0;
	}
};

struct redis_pending_summary
{
	string smallest_id;
	string greatest_id;
	std::vector<redis_pending_consumer> consumers;

	bool empty(void) const
	{
		return consumers.empty();
	}

	size_t size(void) const
	{
		return consumers.size();
	}
};

struct redis_pending_message
{
	string id;
	string consumer;
	unsigned long long elapsed;
	size_t delivered;

	redis_pending_message(void)
	{
		elapsed   = 0;
		delivered = 0;
	}
};

struct redis_pending_detail
{
	std::map<string, redis_pending_message> messages;

	bool empty(void) const
	{
		return messages.empty();
	}

	size_t size(void) const
	{
		return messages.size();
	}
};

class ACL_CPP_API redis_stream : virtual public redis_command
{
public:
	redis_stream(void);
	redis_stream(redis_client* conn);
	redis_stream(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_stream(redis_client_cluster* cluster, size_t max_conns);

	virtual ~redis_stream(void);

	/////////////////////////////////////////////////////////////////////

	/**
	 * appends the specified stream entry to the stream at the specified key
	 * @param key {const char*} the specified key of the stream
	 * @param fields {const std::map<string, string>&} holds all the entries
	 *  to be appended to the stream, the map's key is the entry's name,
	 *  and the value is the entry's value
	 * @param result {string&} will hold the message-id of the added entry
	 * @param id {const char*} a stream entry ID identifies a given entry
	 *  inside a stream, default "*" mean that redis-server will choose
	 *  one ID internal. When the user specified and explicit ID, the ID's
	 *  format is look like 1526919030474-55 that includes two numbers
	 *  separated by '-', the minimum valid ID is 0-1
	 *  param maxlen {size_t} if > 0, limit the size of the stream
	 * @return {bool} return true if entry was added successfully, or some
	 *  error happened which the error reason can be acquied by calling
	 *  result_error() of the base class redis_command.
	 */
	bool xadd(const char* key, const std::map<string, string>& fields,
		string& result, const char* id = "*");
	bool xadd(const char* key, const std::vector<string>& names,
		const std::vector<string>& values,
		string& result, const char* id = "*");
	bool xadd(const char* key, const std::vector<const char*>& names,
		const std::vector<const char*>& values, string& result,
		const char* id = "*");
	bool xadd(const char* key, const char* names[], const size_t names_len[],
		const char* values[], const size_t values_len[], size_t argc,
		string& result, const char* id = "*");
	bool xadd_with_maxlen(const char* key, size_t maxlen,
		const std::map<string, string>& fields, string& result,
		const char* id = "*");

	/**
	 * returns the number of entries inside a stream.
	 * @param key {const char*} the specified key of the stream
	 * @return {int} value >= 0 if the command was executed correctly, -1
	 *  will returned if some error happened.
	 */
	int  xlen(const char* key);

	/**
	 * removes the specified entries from a stream, and returns the number
	 * of entries deleted, that may be different from the number of IDs
	 * passed to the command in case certain IDs do not exist.
	 * @param key {const char*} the specified key of the stream
	 * @param id {const char*} a stream entry ID look like 1526919030474-55
	 * @return {int} return the number of entries actually deleted, if some
	 *  error happened -1 will be returned.
	 */
	int  xdel(const char* key, const char* id);

	/**
	 * removes some entries with the specified IDs, and returns the number
	 * of entries deleted.
	 * @param key {const char*}
	 * @param ids {const std::vector<string>&} holds the entries' IDs to
	 *  be deleted
	 * @return {int}
	 */
	int  xdel(const char* key, const std::vector<string>& ids);
	int  xdel(const char* key, const std::vector<const char*>& ids);

	/**
	 * trims the stream to a given number of items, evicting older items
	 * (items with lower IDs) if needed.
	 * @param key {const char*}
	 * @param maxlen {size_t} specify the latest exactly items to be deleted
	 * @param tilde {bool} if true, the number of items to be deleted is
	 *  not exactly equal the maxlen, the real number maybe more than the
	 *  maxlen with a few tens, but never less than the maxlen
	 * @return return the number of entries deleted from the stream
	 */
	int  xtrim(const char* key, size_t maxlen, bool tilde = false);

	/////////////////////////////////////////////////////////////////////

	/**
	 * read data from one or multiple streams, only returning entries with
	 * an ID greater than the last received ID reported by the caller.
	 * @param messages {redis_stream_messages&} will hold the read's items,
	 *  redis_stream_messages defined above
	 * @param streams {const std::map<string, string>&} holds the specified
	 *  streams' keys to be read by users
	 * @param count {size_t} specifies the max count of items to be read,
	 *  no limit when 0 was set
	 * @param block {ssize_t} specifies the read timeout, block if 0 set,
	 *  no-block if -1 set
	 * @return {bool} return the status of executing the xread command
	 */
	bool xread(redis_stream_messages& messages,
		const std::map<string, string>& streams,
		size_t count = 1000, ssize_t block = 0);

	/**
	 * the XREADGROUP command is a special version of the XREAD command
	 * with support for consumer groups.
	 * @param messages {redis_stream_messages&}
	 * @param group {const char*} the consumer group
	 * @param consumer {const char*} the consumer belonging to the group
	 * @param streams {const std::map<string, string>&} holds the streams'
	 *  keys and IDs for each streams, the map's key is the stream's key
	 *  and the map's value is the stream's ID option, which can be one
	 *  of the following two:
	 *  1. The special > ID, which means that the consumer want to receive
	 *     only messages that were never delivered to any other consumer.
	 *     It just means, give me new messages.
	 *  2. Any other ID, that is, 0 or any other valid ID or incomplete ID
	 *     (just the millisecond time part), will have the effect of
	 *     returning entries that are pending for the consumer sending the
	 *     command. So basically if the ID is not >, then the command will
	 *     just let the client access its pending entries: delivered to it,
	 *     but not yet acknowledged.
	 * @param count {size_t}
	 * @param block {ssize_t} set the blocked timeout waiting for messages,
	 *  if block is 0, will block until getting one message at least;
	 *  if block is -1, don't block for messages.
	 * @param noack {bool} The NOACK subcommand can be used to avoid adding
	 *  the message to the PEL in cases where reliability is not a
	 *  requirement and the occasional message loss is acceptable. This is
	 *  equivalent to acknowledging the message when it is read.
	 * @return {bool} return the status of xreadgroup command
	 */
	bool xreadgroup(redis_stream_messages& messages, const char* group,
		const char* consumer, const std::map<string, string>& streams,
		size_t count = 1000, ssize_t block = 0, bool noack = false);

	/**
	 * the XREADGROUP with NOACK subcommand for reading messages.
	 * @param messages {redis_stream_messages&}
	 * @param group {const char*}
	 * @param consumer {const char*}
	 * @param streams {const std::map<string, string>&}
	 * @param count {size_t}
	 * @param block {ssize_t}
	 * @return {bool}
	 */
	bool xreadgroup_with_noack(redis_stream_messages& messages,
		const char* group, const char* consumer,
		const std::map<string, string>& streams,
		size_t count = 1000, ssize_t block = 0);

	/**
	 * The command returns the stream entries matching a given range of IDs.
	 * @param messages {redis_stream_messages&}
	 * @param key {const char*}
	 * @param start {const char*} the start ID of the query interval;
	 *  '-' means starting from the minimum ID possible inside a stream
	 * @param end {const char*} the end ID of the query interval;
	 *  '+' means the end of the maximum ID possible inside a stream
	 * @param count {size_t} reduce the number of entries reported
	 * @return {bool}
	 */
	bool xrange(redis_stream_messages& messages, const char* key,
		const char* start = "-", const char* end = "+",
		size_t count = 1000);

	/**
	 * Return a range of elements in a stream, with IDs matching the
	 * specified IDs interval, in reverse order (from greater to smaller
	 * IDs) compared to XRANGE.
	 * @param messages {redis_stream_messages&}
	 * @param key {const char*}
	 * @param start {const char*} start with the higher ID
	 * @param end (const char*} end with the lower ID
	 * @param count {size_t}
	 * @return {bool}
	 */
	bool xrevrange(redis_stream_messages& messages, const char* key,
		const char* start = "+", const char* end = "-",
		size_t count = 1000);

	/////////////////////////////////////////////////////////////////////

	/**
	 * In the context of a stream consumer group, this command changes
	 * the ownership of a pending message, so that the new owner is
	 * the consumer specified as the command argument. 
	 * @param messages {std::vector<redis_stream_message>&} holds the
	 *  messages been XLAIMed
	 * @param key {const char*}
	 * @param group {const char*}
	 * @param consumer {const char*}
	 * @param min_idle_time {long}
	 * @param ids {const std::vector<string>&} the IDs to be XCLAIMed
	 * @param idle {size_t}
	 * @param time_ms {long long}
	 * @param retry_count {int}
	 * @param force {bool}
	 * @return {bool}
	 */
	bool xclaim(std::vector<redis_stream_message>& messages,
		const char* key, const char* group, const char* consumer,
		long min_idle_time, const std::vector<string>& ids,
		size_t idle = 0, long long time_ms = -1,
		int retry_count = -1, bool force = false);

	/**
	 * XCLAIM with the JUSTID subcommand
	 */
	bool xclaim_with_justid(std::vector<string>& messages_ids,
		const char* key, const char* group, const char* consumer,
		long min_idle_time, const std::vector<string>& ids,
		size_t idle = 0, long long time_ms = -1,
		int retry_count = -1, bool force = false);

	/////////////////////////////////////////////////////////////////////

	/**
	 * Removes one message from the pending entries list (PEL) of
	 * a stream consumer group.
	 * @param key {const char*}
	 * @param group {const char*}
	 * @param id {const char*}
	 * @return {int} return integer >= 0 if ok, -1 if error
	 */
	int  xack(const char* key, const char* group, const char* id);

	/**
	 * Removes one or multiple message from the pending entries list (PEL)
	 * of a stream consumer group.
	 * @param key {const char*}
	 * @param group {const char*}
	 * @param ids {const std::vector<string>&}
	 * @return {int} return count of messages been acked, return -1 if error
	 */
	int  xack(const char* key, const char* group,
		const std::vector<string>& ids);
	int  xack(const char* key, const char* group,
		const std::vector<const char*>& ids);
	int  xack(const char* key, const char* group,
		const std::list<string>& ids, size_t size);
	int  xack(const char* key, const char* group,
		const std::list<const char*>& ids, size_t size);

	/////////////////////////////////////////////////////////////////////

	/**
	 * The XPENDING command with SUMMARY subcommand.
	 * @param key {const char*}
	 * @param group {const char*}
	 * @param result {redis_pending_summary&} defined above
	 * @return {bool}
	 */
	bool xpending_summary(const char* key, const char* group,
		redis_pending_summary& result);

	/**
	 * The XPENDING command with DETAIL subcommand.
	 * @param result {redis_pending_summary&} defined above
	 * @param key {const char*}
	 * @param group {const char*}
	 * @param start_id {const char*}
	 * @param end_id {const char*}
	 * @param count {size_t} limit the max count to be saved in result
	 * @param consumer {const char*}
	 * @return {bool}
	 */
	bool xpending_detail(redis_pending_detail& result,
		const char* key, const char* group,
		const char* start_id = "-", const char* end_id = "+",
		size_t count = 1, const char* consumer = NULL);

	/////////////////////////////////////////////////////////////////////

	/**
	 * The XGROUP command with the subcommand HELP
	 * @param result {std::vector<string>&} will hold the result
	 * @return {bool}
	 */
	bool xgroup_help(std::vector<string>& result);

	/**
	 * The XGROUP command with the subcommand CREATE
	 * @param key {const char*}
	 * @param group {const char*}
	 * @param id {const char*} the ID of the last item in the stream to
	 *  consider already delivered, "$" means the ID of the last item
	 *  in the stream
	 * @param mkstream {bool} when mkstream is true, the stream with the
	 *  specified key will be created if the stream doesn't exist
	 * @return {bool}
	 */
	bool xgroup_create(const char* key, const char* group,
		const char* id = "$", bool mkstream = true);

	/**
	 * The XGROUP command with the subcommand DESTROY. With this command,
	 * the consumer group will be destroyed even if there are active
	 * consumers and pending messages, so make sure to call this command
	 * only when really needed.
	 * @param key {const char*}
	 * @param group {const char*}
	 * @return {int} return the number of pending messages for the
	 *  specified group, return -1 if error
	 */
	int  xgroup_destroy(const char* key, const char* group);
	bool xgroup_setid(const char* key, const char* group,
		const char* id = "$");

	/**
	 * The XGROUP command with the subcommand DELCONSUMER. With this command,
	 * just remove a given consumer from a consumer group.
	 * @param key {const char*}
	 * @param group {const char*}
	 * @param consumer {const char*}
	 * @return {int} return the number of the pending messages for the
	 *  specified consumer, return -1 if error
	 */
	int  xgroup_delconsumer(const char* key, const char* group,
		const char* consumer);

	/////////////////////////////////////////////////////////////////////

	/**
	 * The XINFO command with the subcommand HELP
	 * @param result {std::vector<string>&}
	 * @return {bool}
	 */
	bool xinfo_help(std::vector<string>& result);

	/**
	 * The XINFO command with the subcommand CONSUMERS. With this command,
	 * every consumer in a specific consumer group can be got.
	 * @param key {const char*}
	 * @param group {const char*}
	 * @param result {std::map<string, redis_xinfo_consumer>&}
	 * @return {bool}
	 */
	bool xinfo_consumers(const char* key, const char* group,
		std::map<string, redis_xinfo_consumer>& result);

	/**
	 * The XINFO command with the subcommand GROUPS.
	 * @param key {const char*}
	 * @param result {std::map<string, redis_xinfo_group>&}
	 * @return {bool}
	 */
	bool xinfo_groups(const char* key,
		std::map<string, redis_xinfo_group>& result);

	/**
	 * The XINFO command with the subcommand STREAM. In this form the
	 * command returns general information about the stream stored
	 * at the specified key.
	 * @param key {const char*}
	 * @param result {redis_stream_info&} devined about
	 * @return {bool}
	 */
	bool xinfo_stream(const char* key, redis_stream_info& result);

	/////////////////////////////////////////////////////////////////////

private:
	void build(const char* cmd, const char* key, const char* id,
		const std::map<string, string>& fields);
	void build(const char* cmd, const char* key, const char* id,
		const std::vector<string>& names,
		const std::vector<string>& values);
	void build(const char* cmd, const char* key, const char* id,
		const std::vector<const char*>& names,
		const std::vector<const char*>& values);
	void build(const char* cmd, const char* key, const char* id,
		const char* names[], const size_t names_len[],
		const char* values[], const size_t values_len[], size_t argc);
	void build(const std::map<string, string>& streams, size_t i,
		size_t count, ssize_t block, bool noack = false);
	void xread_build(const std::map<string, string>& streams,
		size_t count, ssize_t block);
	void xreadgroup_build(const char* group, const char* consumer,
		const std::map<string, string>& streams,
		size_t count, ssize_t block, bool noack);
	bool get_results(redis_stream_messages& messages);
	bool get_messages(const redis_result& rr, redis_stream_messages& messages);
	bool get_one_message(const redis_result& rr, redis_stream_message& message);
	bool range(redis_stream_messages& messages, const char* cmd,
	     	const char* key, const char* start, const char* end, size_t count);

	bool get_one_consumer(const redis_result& rr, redis_xinfo_consumer& consumer);
	bool get_one_group(const redis_result& rr, redis_xinfo_group& group);
	bool get_pending_consumer(const redis_result& rr,
		redis_pending_consumer& consumer);
	bool get_pending_message(const redis_result& rr,
		redis_pending_message& message);

	void xclaim_build(const char* key, const char* group,
		const char* consumer, long min_idle_time,
		const std::vector<string>& ids, size_t idle, long long time_ms,
		int retry_count, bool force, bool justid);

};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
