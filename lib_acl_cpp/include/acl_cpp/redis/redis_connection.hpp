#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class redis_client;

/**
 * redis Connection 类，包含命令如下：
 * AUTH、ECHO、PING、QUIT、SELECT
 */
class ACL_CPP_API redis_connection : virtual public redis_command
{
public:
	redis_connection();
	redis_connection(redis_client* conn);
	redis_connection(redis_cluster* cluster, size_t max_conns);
	virtual ~redis_connection();

	/////////////////////////////////////////////////////////////////////

	/**
	 * 连接至 redis-server 时进行身份验证
	 * @param passwd {const char*} 在 redis 配置文件中指定的认证密码
	 * @return {bool} 身份认证是否成功，返回 false 表示认证失败或操作失败
	 */
	bool auth(const char* passwd);

	/**
	 * 选择 redis-server 中的数据库 ID
	 * @param dbnum {int} redis 数据库 ID
	 * @return {bool} 操作是否成功
	 */
	bool select(int dbnum);

	/**
	 * 探测 redis 连接是否正常
	 * @return {bool} 连接是否正常
	 */
	bool ping();

	/**
	 * 测试用命令，让 redis-server 回显给定字符串
	 * @return {bool} 操作是否成功
	 */
	bool echo(const char* s);

	/**
	 * 关闭 redis 连接
	 * @return {bool}
	 */
	bool quit();
};

} // namespace acl
