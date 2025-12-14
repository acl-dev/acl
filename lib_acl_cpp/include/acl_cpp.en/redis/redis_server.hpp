#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include "../stdlib/string.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

class ACL_CPP_API redis_server : virtual public redis_command {
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_server();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_server(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	redis_server(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_server(redis_client_cluster* cluster, size_t max_conns);

	redis_server(redis_client_pipeline* pipeline);

	virtual ~redis_server();

	/////////////////////////////////////////////////////////////////////

	
	/**
	 * Execute an AOF file rewrite operation. Rewrite creates a volume-optimized version of current AOF file.
	 * Even if BGREWRITEAOF execution fails, there will be no data loss, because old AOF file will not be modified
	 * before BGREWRITEAOF succeeds
	 * @return {bool}
	 */
	bool bgrewriteaof();

	/**
	 * Asynchronously save current database data to disk in background. BGSAVE command returns
	 * OK immediately after execution, then Redis forks a new child process. Original Redis process (parent process)
	 * continues to handle client requests, while child process is responsible for saving data to disk, then exits. Clients can
	 * use LASTSAVE command to check related information and determine whether BGSAVE command executed successfully
	 * @return {bool}
	 */
	bool bgsave();

	/**
	 * Return name set for connection by CLIENT SETNAME command
	 * @param buf {string&} Store result. If not set, it is empty
	 * @return {bool} Returns false indicates connection name was not set or error occurred
	 */
	bool client_getname(string& buf);

	/**
	 * Close client at address ip:port
	 * @param addr {const char*} Client connection address, format: ip:port
	 * @return {bool} Whether successful. Returns false indicates connection does not exist or error occurred
	 */
	bool client_kill(const char* addr);

	/**
	 * Return information and statistics of all clients connected to server
	 * @param buf {string&} Store result
	 * @return {int} Returns result data length. -1 indicates error
	 */
	int client_list(string& buf);

	/**
	 * Assign a name to current connection. This name will appear in CLIENT LIST command's results.
	 * When Redis application has connection leaks, setting name for connection is a good debug method
	 * @param name {const char*} Connection name. This name does not need to be unique
	 * @return {bool} Whether operation was successful
	 */
	bool client_setname(const char* name);

	/**
	 * Command used to get configuration parameters of running Redis server
	 * @param parameter {const char*} Configuration parameter name
	 * @param out {std::map<string, string>&} Store result, composed of name-value pairs.
	 *  Because parameter supports fuzzy matching, result set may contain multiple parameter items
	 * @return {int} Number of "parameter-value" pairs in result. -1 indicates error
	 */
	int config_get(const char* parameter, std::map<string, string>& out);

	/**
	 * Reset some statistics in INFO command
	 * @return {bool} Whether reset was successful
	 */
	bool config_resetstat();

	/**
	 * Rewrite redis.conf file specified when starting Redis server
	 * @return {bool} Whether rewriting configuration was successful
	 */
	bool config_rewrite();

	/**
	 * Dynamically adjust Redis server's configuration without restarting service
	 * @param name {const char*} Configuration parameter name
	 * @param value {const char*} Configuration parameter value
	 * @return {bool} Whether successful
	 */
	bool config_set(const char* name, const char* value);

	/**
	 * Return number of keys in current database
	 * @return {int} Returns -1 indicates error
	 */
	int dbsize();

	/**
	 * Clear all data of Redis server (delete all keys in all databases)
	 * @return {bool}
	 *  Note: Use this command with caution to avoid misoperation
	 */
	bool flushall();

	/**
	 * Clear all keys in current database
	 * @return {bool}
	 *  Note: Use this command with caution to avoid misoperation
	 */
	bool flushdb();

	/**
	 * Return various information and statistics about Redis server
	 * @param buf {string&} Store result
	 * @return {int} Returns length of stored data
	 */
	int info(string& buf);

	/**
	 * Return various information and statistics about Redis server
	 * @param out {std::map<string, string>&} Store result
	 * @return {int} Returns number of stored data entries. -1 indicates error
	 */
	int info(std::map<string, string>& out);

	/**
	 * Return time when Redis last successfully saved data to disk, represented in UNIX timestamp format
	 * @return {time_t}
	 */
	time_t lastsave();

	/**
	 * Real-time print commands received by Redis server, for debugging. After calling this command, can call following
	 * get_command method in a loop to get commands received by server
	 * @return {bool}
	 */
	bool monitor();

	/**
	 * After calling monitor method, need to call this method to get commands received by server. Can call this method in a loop
	 * to continuously get commands received by server
	 * @param buf {string&} Store result
	 * @return {bool}
	 */
	bool get_command(string& buf);

	/**
	 * Command executes a synchronous save operation, saving all data snapshots of current Redis instance
	 * to disk in RDB file format
	 * @return {bool}
	 */
	bool save();

	/**
	 * Stop all client connections, save data to disk, then server program exits
	 * @param save_data {bool} Whether to save data to disk before exiting
	 */
	void shutdown(bool save_data = true);

	/**
	 * Convert current server to slave server of specified server
	 * @param ip {const char*} IP of specified server
	 * @param port {int} Port of specified server
	 * @return {bool} Whether successful
	 */
	bool slaveof(const char* ip, int port);

	/**
	 * Query slow operation log
	 * @param number {int} When > 0, limits number of log entries, otherwise lists all logs
	 * @return {const redis_result*}
	 */
	const redis_result* slowlog_get(int number = 0);

	/**
	 * Can view number of current logs
	 * @return {int}
	 */
	int slowlog_len();

	/**
	 * Can clear slow log
	 * @return {bool}
	 */
	bool slowlog_reset();

	/**
	 * Return current server time
	 * @param stamp {time_t&} Store timestamp (represented in UNIX timestamp format)
	 * @param escape {int*} Store number of microseconds elapsed in current second
	 */
	bool get_time(time_t& stamp, int& escape);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

