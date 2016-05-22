#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <map>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class redis_client;
class redis_result;

class ACL_CPP_API redis_server : virtual public redis_command
{
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_server(void);

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_server(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*, size_t)
	 */
	redis_server(redis_client_cluster* cluster, size_t max_conns = 0);

	virtual ~redis_server(void);

	/////////////////////////////////////////////////////////////////////

	
	/**
	 * 执行一个 AOF文件 重写操作。重写会创建一个当前 AOF 文件的体积优化版本；
	 * 即使 BGREWRITEAOF 执行失败，也不会有任何数据丢失，因为旧的 AOF 文件在
	 * BGREWRITEAOF 成功之前不会被修改
	 * @return {bool}
	 */
	bool bgrewriteaof(void);

	/**
	 * 在后台异步(Asynchronously)保存当前数据库的数据到磁盘，BGSAVE 命令执行之后
	 * 立即返回 OK ，然后 Redis fork 出一个新子进程，原来的 Redis 进程(父进程)
	 * 继续处理客户端请求，而子进程则负责将数据保存到磁盘，然后退出；客户端可以通过
	 * LASTSAVE 命令查看相关信息，判断 BGSAVE 命令是否执行成功
	 * @return {bool}
	 */
	bool bgsave(void);

	/**
	 * 返回 CLIENT SETNAME 命令为连接设置的名字
	 * @param buf {string&} 存储结果，如果没有设置则为空
	 * @return {bool} 返回 false 则表明没有设置连接名字或出错
	 */
	bool client_getname(string& buf);

	/**
	 * 关闭地址为 ip:port 的客户端
	 * @param addr {const char*} 客户端连接地址，格式：ip:port
	 * @return {bool} 是否成功，返回 false 表明连接不存在或出错
	 */
	bool client_kill(const char* addr);

	/**
	 * 返回所有连接到服务器的客户端信息和统计数据
	 * @param buf {string&} 存储结果
	 * @return {int} 返回结果数据长度，-1 表示出错
	 */
	int client_list(string& buf);

	/**
	 * 为当前连接分配一个名字，该名字会出现在 CLIENT LIST 命令的结果中；
	 * 在 Redis 应用程序发生连接泄漏时，为连接设置名字是一种很好的 debug 手段
	 * @param name {const char*} 连接名字，该名字不需要唯一性
	 * @return {bool} 操作是否成功
	 */
	bool client_setname(const char* name);

	/**
	 * 命令用于取得运行中的 Redis 服务器的配置参数
	 * @param parameter {const char*} 配置参数名
	 * @param out {std::map<string, string>&} 存储结果，由 name-value 组成，
	 *  因为 parameter 支持模糊匹配，所以有可能返回的结果集中会有多个参数项
	 * @return {int} 结果 "参数-值" 的个数，-1 表示出错
	 */
	int config_get(const char* parameter, std::map<string, string>& out);

	/**
	 * 重置 INFO 命令中的某些统计数据
	 * @return {bool} 重置是否成功
	 */
	bool config_resetstat(void);

	/**
	 * 对启动 Redis 服务器时所指定的 redis.conf 文件进行改写
	 * @return {bool} 重写配置是否成功
	 */
	bool config_rewrite(void);

	/**
	 * 动态地调整 Redis 服务器的配置而无需重启服务
	 * @param name {const char*} 配置参数名
	 * @param value {const char*} 配置参数值
	 * @return {bool} 是否成功
	 */
	bool config_set(const char* name, const char* value);

	/**
	 * 返回当前数据库的 key 的数量
	 * @return {int} 返回 -1 表示出错
	 */
	int dbsize(void);

	/**
	 * 清空整个 Redis 服务器的数据(删除所有数据库的所有 key )
	 * @return {bool}
	 *  注：此命令要慎用，以免造成误操作
	 */
	bool flushall(void);

	/**
	 * 清空当前数据库中的所有 key
	 * @return {bool}
	 *  注：此命令要慎用，以免造成误操作
	 */
	bool flushdb(void);

	/**
	 * 返回关于 Redis 服务器的各种信息和统计数值
	 * @param buf {string&} 存储结果
	 * @return {int} 返回所存储的数据长度
	 */
	int info(string& buf);

	/**
	 * 返回关于 Redis 服务器的各种信息和统计数值
	 * @param out {std::map<string, string>&} 存储结果
	 * @return {int} 返回所存储的数据条目数量, -1 表示出错
	 */
	int info(std::map<string, string>& out);

	/**
	 * 返回最近一次 Redis 成功将数据保存到磁盘上的时间，以 UNIX 时间戳格式表示
	 * @return {time_t}
	 */
	time_t lastsave(void);

	/**
	 * 实时打印出 Redis 服务器接收到的命令，调试用; 调用本命令后可以循环调用下面的
	 * get_command 方法获得服务器收到的命令
	 * @return {bool}
	 */
	bool monitor(void);

	/**
	 * 调用 monitor 方法后需要调用本方法获得服务器收到的命令，可以循环调用本方法
	 * 以便于不断地获得服务器收到的命令
	 * @param buf {string&} 存储结果
	 * @return {bool}
	 */
	bool get_command(string& buf);

	/**
	 * 命令执行一个同步保存操作，将当前 Redis 实例的所有数据快照(snapshot)
	 * 以 RDB 文件的形式保存到硬盘
	 * @return {bool}
	 */
	bool save(void);

	/**
	 * 停止所有客户端连接将数据保存至磁盘后服务器程序退出
	 * @param save_data {bool} 是否在退出前保存数据至磁盘
	 */
	void shutdown(bool save_data = true);

	/**
	 * 将当前服务器转变为指定服务器的从属服务器
	 * @param ip {const char*} 指定服务器的 IP
	 * @param port {int} 指定服务器的端口
	 * @return {bool} 是否成功
	 */
	bool slaveof(const char* ip, int port);

	/**
	 * 查询较慢的操作日志
	 * @param number {int} 大于 0 时则限定日志条数，否则列出所有日志
	 * @return {const redis_result*}
	 */
	const redis_result* slowlog_get(int number = 0);

	/**
	 * 可以查看当前日志的数量
	 * @return {int}
	 */
	int slowlog_len(void);

	/**
	 * 可以清空 slow log
	 * @return {bool}
	 */
	bool slowlog_reset(void);

	/**
	 * 返回当前服务器时间
	 * @param stamp {time_t&} 存储时间截(以 UNIX 时间戳格式表示)
	 * @param escape {int*} 存储当前这一秒钟已经逝去的微秒数
	 */
	bool get_time(time_t& stamp, int& escape);
};

} // namespace acl
