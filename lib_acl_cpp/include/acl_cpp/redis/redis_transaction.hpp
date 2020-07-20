#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "../stdlib/string.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

class redis_client;
class redis_result;

class ACL_CPP_API redis_transaction : virtual public redis_command
{
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_transaction(void);

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_transaction(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	redis_transaction(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_transaction(redis_client_cluster* cluster, size_t max_conns);

	virtual ~redis_transaction(void);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 监视一个(或多个) key ，如果在事务执行之前这个(或这些) key 被其他命令所改动，
	 * 那么事务将被打断
	 * watch the given keys to determine execution of the MULTI/EXEC
	 * block, before EXEC some of the given keys were changed outer,
	 * the transaction will break
	 * @param keys {const std::vector<string>&} key 集合
	 *  the given keys collection
	 * @return {bool} 操作是否成功，即使 key 集合中的有 key 不存在也会返回成功
	 *  if success of this operation
	 */
	bool watch(const std::vector<string>& keys);

	/**
	 * 取消 WATCH 命令对所有 key 的监视
	 * forget about all watched keys
	 * @return {bool} 操作是否成功
	 * if success of this operation
	 */
	bool unwatch(void);

	/**
	 * 标记一个事务块的开始，事务块内的多条命令会按照先后顺序被放进一个队列当中，
	 * 最后由 EXEC 命令原子性(atomic)地执行
	 * mark the start of a transaction block
	 * @return {bool} 操作是否成功
	 *  if success of this operation
	 */
	bool multi(void);

	/**
	 * 执行所有事务块内的命令，假如某个(或某些) key 正处于 WATCH 命令的监视之下，
	 * 且事务块中有和这个(或这些) key 相关的命令，那么 EXEC 命令只在这个(或这些)
	 * key 没有被其他命令所改动的情况下执行并生效，否则该事务被打断(abort)；
	 * 在执行本条命令成功后，可以调用下面的 get_size()/get_child() 获得每条命令的
	 * 操作结果
	 * execute all commands issued after MULTI
	 * @return {bool} 操作是否成功
	 *  if success of this operation
	 */
	bool exec(void);

	/**
	 * 取消事务，放弃执行事务块内的所有命令，如果正在使用 WATCH 命令监视某个(或某些)
	 * key，那么取消所有监视，等同于执行命令 UNWATCH
	 * discard all commands issued after MULTI
	 * @return {bool}
	 */
	bool discard(void);

	/**
	 * 在 multi 和 exec 之间可多次调用本函数执行多条 redis 客户端命令
	 * run one command between MULTI and EXEC
	 * @param cmd {const char*} redis 命令
	 *  the command
	 * @param argv {const char* []} 参数数组
	 *  the args array associate with the command
	 * @param lens [const size_t []} 参数的长度数组
	 *  the length array of the args array
	 * @param argc {size_t} 参数数组的长度
	 *  the length of the array for args
	 * @return {bool} 操作是否成功
	 *  if successful
	 */
	bool run_cmd(const char* cmd, const char* argv[],
		const size_t lens[], size_t argc);

	/**
	 * 在 multi 和 exec 之间多次调用本函数执行多条 redis 客户端命令
	 * run one command between MULTI and exec, this function can be
	 * called more than once
	 * @param cmd {const char*} redis 命令
	 *  the redis command
	 * @param args {const std::vector<string>&} 参数数组
	 *  the args array for the command
	 * @return {bool} 操作是否成功
	 *  if successful
	 */
	bool run_cmd(const char* cmd, const std::vector<string>& args);

	/**
	 * 在成功调用 exec 后调用本函数获得操作结果数组的长度
	 * get the result array's length after EXEC
	 * @return {size_t}
	 */
	size_t get_size(void) const;

	/**
	 * 获取指定下标的对应的命令的执行结果对象
	 * get the result of the given subscript
	 * @param i {size_t} 命令执行结果在结果数组中的下标
	 *  the given subscript
	 * @param cmd {string*} 该参数非空时存放对应的 redis 命令
	 *  if not NULL, it will store the command of the given subscript
	 * @return {const redis_result*} 执行某条命令的结果，当 i 越界时返回 NULL
	 *  return the result of one command, NULL if i was out of bounds
	 */
	const redis_result* get_child(size_t i, string* cmd) const;

	/**
	 * 获得当前事务所重的命令集合
	 * get all the commands issued between MULTI and EXEC
	 * @return {const std::vector<string>&}
	 */
	const std::vector<string>& get_commands(void) const
	{
		return cmds_;
	}

private:
	std::vector<string> cmds_;
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
