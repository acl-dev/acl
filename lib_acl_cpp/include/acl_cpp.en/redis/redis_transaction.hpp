#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "../stdlib/string.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

class ACL_CPP_API redis_transaction : virtual public redis_command {
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_transaction();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	explicit redis_transaction(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	explicit redis_transaction(redis_client_cluster* cluster);

	explicit redis_transaction(redis_client_pipeline* pipeline);

	ACL_CPP_DEPRECATED
	redis_transaction(redis_client_cluster* cluster, size_t max_conns);

	virtual ~redis_transaction();

	/////////////////////////////////////////////////////////////////////

	/**
	 * Watch one (or more) key(s). If this (or these) key(s) are modified by other
	 * commands before transaction execution,
	 * then the transaction will be aborted
	 * watch the given keys to determine execution of the MULTI/EXEC
	 * block, before EXEC some of the given keys were changed outer,
	 * the transaction will break
	 * @param keys {const std::vector<string>&} Key collection
	 *  the given keys collection
	 * @return {bool} Whether operation was successful. Even if some keys in key
	 * collection do not exist, will still return success
	 *  if success of this operation
	 */
	bool watch(const std::vector<string>& keys);

	/**
	 * Cancel WATCH command's monitoring of all keys
	 * forget about all watched keys
	 * @return {bool} Whether operation was successful
	 * if success of this operation
	 */
	bool unwatch();

	/**
	 * Mark the start of a transaction block. Multiple commands within the
	 * transaction block will be placed in a queue in order,
	 * and finally executed atomically by EXEC command
	 * mark the start of a transaction block
	 * @return {bool} Whether operation was successful
	 *  if success of this operation
	 */
	bool multi();

	/**
	 * Execute all commands within the transaction block. If some key(s) are being
	 * monitored by WATCH command,
	 * and there are commands related to this (or these) key(s) in the transaction
	 * block, then EXEC command will only execute and take effect
	 * when this (or these) key(s) have not been modified by other commands,
	 * otherwise the transaction is aborted;
	 * After successfully executing this command, can call get_size()/get_child()
	 * below to get operation result of each command
	 * execute all commands issued after MULTI
	 * @return {bool} Whether operation was successful
	 *  if success of this operation
	 */
	bool exec();

	/**
	 * Cancel transaction, abandon execution of all commands within the transaction
	 * block. If currently using WATCH command to monitor some key(s),
	 * then cancel all monitoring, equivalent to executing UNWATCH command
	 * discard all commands issued after MULTI
	 * @return {bool}
	 */
	bool discard();

	/**
	 * Can call this function multiple times between multi and exec to execute
	 * multiple redis client commands
	 * run one command between MULTI and EXEC
	 * @param cmd {const char*} Redis command
	 *  the command
	 * @param argv {const char* []} Parameter array
	 *  the args array associate with the command
	 * @param lens [const size_t []} Length array of parameters
	 *  the length array of the args array
	 * @param argc {size_t} Length of parameter array
	 *  the length of the array for args
	 * @return {bool} Whether operation was successful
	 *  if successful
	 */
	bool run_cmd(const char* cmd, const char* argv[],
		const size_t lens[], size_t argc);

	/**
	 * Call this function multiple times between multi and exec to execute multiple
	 * redis client commands
	 * run one command between MULTI and exec, this function can be
	 * called more than once
	 * @param cmd {const char*} Redis command
	 *  the redis command
	 * @param args {const std::vector<string>&} Parameter array
	 *  the args array for the command
	 * @return {bool} Whether operation was successful
	 *  if successful
	 */
	bool run_cmd(const char* cmd, const std::vector<string>& args);

	/**
	 * After successfully calling exec, call this function to get the length of
	 * operation result array
	 * get the result array's length after EXEC
	 * @return {size_t}
	 */
	size_t get_size() const;

	/**
	 * Get execution result object of the command corresponding to the specified
	 * index
	 * get the result of the given subscript
	 * @param i {size_t} Index of command execution result in result array
	 *  the given subscript
	 * @param cmd {string*} When this parameter is not NULL, stores the
	 * corresponding redis command
	 *  if not NULL, it will store the command of the given subscript
	 * @return {const redis_result*} Result of executing a command. Returns NULL
	 * when i is out of bounds
	 *  return the result of one command, NULL if i was out of bounds
	 */
	const redis_result* get_child(size_t i, string* cmd) const;

	/**
	 * Get command collection of current transaction
	 * get all the commands issued between MULTI and EXEC
	 * @return {const std::vector<string>&}
	 */
	const std::vector<string>& get_commands() const {
		return cmds_;
	}

private:
	std::vector<string> cmds_;
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

