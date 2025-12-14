#pragma once
#include "../acl_cpp_define.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

class redis_client;

/**
 * redis Connection class, including commands as below:
 * AUTH, ECHO, PING, QUIT, SELECT
 * redis connection command clss, including as below:
 * AUTH, ECHO, PING, QUIT, SELECT
 */
class ACL_CPP_API redis_connection : virtual public redis_command {
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_connection();

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

	redis_connection(redis_client_pipeline* pipeline);

	virtual ~redis_connection();

	/////////////////////////////////////////////////////////////////////

	/**
	 * Authenticate when connecting to redis-server.
	 * AUTH command to login the redis server.
	 * @param passwd {const char*} Authentication password specified in redis configuration file.
	 *  the password in redis-server configure
	 * @return {bool} Whether authentication was successful. Returns false to indicate authentication failed or operation failed.
	 *  return true if success, or false because auth failed or error.
	 */
	bool auth(const char* passwd);

	/**
	 * Select database ID in redis-server.
	 * SELECT command to select the DB id in redis-server
	 * @param dbnum {int} redis database ID.
	 *  the DB id
	 * @return {bool} Whether operation was successful.
	 *  return true if success, or false for failed.
	 */
	bool select(int dbnum);

	/**
	 * Probe whether redis connection is normal.
	 * PING command for testing if the connection is OK
	 * @return {bool} Whether connection is normal.
	 *  return true if success
	 */
	bool ping();

	/**
	 * Test command, let redis-server echo given string.
	 * ECHO command, request redis-server to echo something.
	 * @return {bool} Whether operation was successful.
	 *  return true if success
	 */
	bool echo(const char* s);

	/**
	 * Close redis connection.
	 * QUIT command to close the redis connection
	 * @return {bool}
	 *  return true if success
	 */
	bool quit();
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
