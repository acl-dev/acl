#pragma once
#include "../acl_cpp_define.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

class redis_client;

/**
 * redis Connection 类，包含命令如下：
 * AUTH、ECHO、PING、QUIT、SELECT
 * redis connection command clss, including as below:
 * AUTH, ECHO, PING, QUIT, SELECT
 */
class ACL_CPP_API redis_connection : virtual public redis_command
{
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_connection(void);

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_connection(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	redis_connection(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_connection(redis_client_cluster* cluster, size_t max_conns);

	virtual ~redis_connection(void);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 连接至 redis-server 时进行身份验证
	 * AUTH command to login the redis server.
	 * @param passwd {const char*} 在 redis 配置文件中指定的认证密码
	 *  the password in redis-server configure
	 * @return {bool} 身份认证是否成功，返回 false 表示认证失败或操作失败
	 *  return true if success, or false because auth failed or error.
	 */
	bool auth(const char* passwd);

	/**
	 * 选择 redis-server 中的数据库 ID
	 * SELECT command to select the DB id in redis-server
	 * @param dbnum {int} redis 数据库 ID
	 *  the DB id
	 * @return {bool} 操作是否成功
	 *  return true if success, or false for failed.
	 */
	bool select(int dbnum);

	/**
	 * 探测 redis 连接是否正常
	 * PING command for testing if the connection is OK
	 * @return {bool} 连接是否正常
	 *  return true if success
	 */
	bool ping();

	/**
	 * 测试用命令，让 redis-server 回显给定字符串
	 * ECHO command, request redis-server to echo something.
	 * @return {bool} 操作是否成功
	 *  return true if success
	 */
	bool echo(const char* s);

	/**
	 * 关闭 redis 连接
	 * QUIT command to close the redis connection
	 * @return {bool}
	 *  return true if success
	 */
	bool quit();
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
