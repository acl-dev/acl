#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_command.hpp"

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
	redis_transaction();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_transaction(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_cluster*， size_t)
	 */
	redis_transaction(redis_cluster* cluster, size_t max_conns);

	virtual ~redis_transaction();

	/////////////////////////////////////////////////////////////////////

	/**
	 * 监视一个(或多个) key ，如果在事务执行之前这个(或这些) key 被其他命令所改动，
	 * 那么事务将被打断
	 * @param keys {const std::vector<string>&} key 集合
	 * @return {bool} 操作是否成功，即使 key 集合中的有 key 不存在也会返回成功
	 */
	bool watch(const std::vector<string>& keys);

	/**
	 * 取消 WATCH 命令对所有 key 的监视
	 * @return {bool} 操作是否成功
	 */
	bool unwatch();

	/**
	 * 标记一个事务块的开始，事务块内的多条命令会按照先后顺序被放进一个队列当中，
	 * 最后由 EXEC 命令原子性(atomic)地执行
	 * @return {bool} 操作是否成功
	 */
	bool multi();

	/**
	 * 执行所有事务块内的命令，假如某个(或某些) key 正处于 WATCH 命令的监视之下，
	 * 且事务块中有和这个(或这些) key 相关的命令，那么 EXEC 命令只在这个(或这些)
	 * key 没有被其他命令所改动的情况下执行并生效，否则该事务被打断(abort)；
	 * 在执行本条命令成功后，可以调用下面的 get_size()/get_child() 获得每条命令的
	 * 操作结果
	 * @return {bool} 操作是否成功
	 */
	bool exec();

	/**
	 * 取消事务，放弃执行事务块内的所有命令，如果正在使用 WATCH 命令监视某个(或某些)
	 * key，那么取消所有监视，等同于执行命令 UNWATCH
	 */
	bool discard();

	/**
	 * 在 multi 和 exec 之间执行多条 redis 客户端命令
	 * @param cmd {const char*} redis 命令
	 * @param argv {const char* []} 参数数组
	 * @param lens [const size_t []} 参数的长度数组
	 * @param argc {size_t} 参数数组的长度
	 * @return {bool} 操作是否成功
	 */
	bool run_cmd(const char* cmd, const char* argv[],
		const size_t lens[], size_t argc);

	/**
	 * 在 multi 和 exec 之间执行多条 redis 客户端命令
	 * @param cmd {const char*} redis 命令
	 * @param args {const std::vector<string>&} 参数数组
	 * @return {bool} 操作是否成功
	 */
	bool run_cmd(const char* cmd, const std::vector<string>& args);

	/**
	 * 在成功调用 exec 后调用本函数获得操作结果数组的长度
	 * @return {size_t}
	 */
	size_t get_size() const;

	/**
	 * 获取指定下标的对应的命令的执行结果对象
	 * @param i {size_t} 命令执行结果在结果数组中的下标
	 * @param cmd {string*} 该参数非空时存放对应的 redis 命令
	 * @return {const redis_result*} 执行的某条命令的结果对象，
	 *  当 i 越界时返回 NULL
	 */
	const redis_result* get_child(size_t i, string* cmd) const;

	/**
	 * 获得当前事务所重的命令集合
	 * @return {const std::vector<string>&}
	 */
	const std::vector<string>& get_commands() const
	{
		return cmds_;
	}

private:
	std::vector<string> cmds_;
};

} // namespace acl
