#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "../stream/socket_stream.hpp"
#include "../connpool/connect_client.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

class string;
class dbuf_pool;
class redis_result;
class redis_request;
class redis_command;
class sslbase_conf;

/**
 * Redis client object network communication class. Through this class, organized redis request commands are sent to redis
 * server, and redis server response results are received. This class inherits from connect_client class, mainly
 * to use connection pool functionality.
 * redis client network IO class. The redis request is sent to server
 * and the server's respond is handled in this class. The class inherits
 * connect_client, which can use the connection pool function.
 */
class ACL_CPP_API redis_client : public connect_client {
public:
	/**
	 * Constructor
	 * constructor
	 * @param addr {const char*} Redis-server listening address
	 *  the redis-server listening addr
	 * @param conn_timeout {int} Timeout for connecting to redis-server (seconds)
	 *  the timeout in seconds to connect the redis-server
	 * @param rw_timeout {int} IO timeout for communicating with redis-server (seconds)
	 *  the network IO timeout in seconds with the redis-server
	 */
	explicit redis_client(const char* addr, int conn_timeout = 60,
		int rw_timeout = 30, bool retry = true);
	~redis_client();

	/**
	 * Set SSL communication configuration handle. Internal default value is NULL. If SSL connection
	 * configuration object is set, internally switches to SSL communication mode
	 * set SSL communication with Redis-server if ssl_conf not NULL
	 * @param ssl_conf {sslbase_conf*}
	 */
	void set_ssl_conf(sslbase_conf* ssl_conf);

	/**
	 * Call this function to set connection password for connecting to redis service
	 * @param pass {const char*}
	 */
	void set_password(const char* pass);

	/**
	 * Set db corresponding to this connection. After connection is established, if specified db value is greater than 0, internally automatically
	 * selects corresponding db. Note: This function is only for non-cluster mode
	 * if db > 0 in no cluster mode, select the db when the connection
	 * is created.
	 * @param dbnum {int}
	 */
	void set_db(int dbnum);

	/**
	 * Get db selected by this connection
	 * get db for the connection
	 * @return {int}
	 */
	int get_db() const {
		return dbnum_;
	}

	/**
	 * Get current connection's server address, i.e., address passed in when redis_client is constructed
	 * @return {const char*}
	 */
	const char* get_addr() const {
		return addr_;
	}

	/**
	 * Before processing each command, whether to require checking socket handle and address matching. When
	 * this option is enabled, will strictly check matching, but will affect performance to some extent, so this setting is only
	 * used in DEBUG runtime scenarios
	 * @param on {bool}
	 */
	void set_check_addr(bool on);

	/**
	 * Determine whether this network connection object has been closed
	 * check if the connection has been finish
	 * @return {bool}
	 */
	bool eof() const;

	/**
	 * Close network connection
	 * close the connection to the redis-server
	 */
	void close();

	/**
	 * Get network connection stream. When connection is closed, internally will automatically reconnect once
	 * get acl::socket_stream from the connection
	 * @param auto_connect {bool} Whether internally needs to automatically connect to server
	 *  if we should connect the redis server automatically
	 * @return {acl::socket_stream*} Returns NULL if connection has been closed
	 *  return NULL if the connection has been closed
	 */
	socket_stream* get_stream(bool auto_connect = true);

	/**
	 * For request data packets, this function sets flag to send as one data packet when assembling request data packets
	 * just for request package, setting flag for sending data with
	 * multi data chunks; this is useful when the request data is large
	 * @param on {bool} When true, request data will not be combined into one data packet for sending
	 *  if true the request data will not be combined one package
	 */
	void set_slice_request(bool on);

	/**
	 * For response data packets, this function sets whether to split redis-server response data into multiple data blocks.
	 * This is useful for large data packets, avoiding allocating one large contiguous memory at once
	 * just for response package, settint flag for receiving data
	 * if split the large response data into multi little chunks
	 * @param on {bool} When true, response data packets will be split
	 *  if true the response data will be splitted into multi little
	 *  data, which is useful for large reponse data for avoiding
	 *  malloc large continuously memory from system.
	 */
	void set_slice_respond(bool on);

	/**
	 * Used for non-sliced sending mode, send request data to redis-server, and read and analyze
	 * response data returned by server
	 * send request to redis-server, and read/anlyse response from server,
	 * this function will be used for no-slice request mode.
	 * @param pool {dbuf_pool*} Memory pool manager object
	 *  memory pool manager
	 * @param req {const string&} Request data packet
	 *  the request package
	 * @param nchildren {size_t} Number of data objects in response data
	 *  the data object number in the server's response data
	 * @return {const redis_result*} Server response object read. Returns NULL on error.
	 *  This object does not need to be manually released, because it is dynamically allocated on pool memory pool object, so when
	 *  pool is released, this result object is also released
	 *  the result object from server's response, NULL will be returned
	 *  when some error happens; the result object needn't be freed
	 *  manually, which was created in the pool object, and will be freed
	 *  when the pool were freed.
	 *  
	 */
	const redis_result* run(dbuf_pool* pool, const string& req,
		size_t nchildren, int* rw_timeout = NULL);

	/**
	 * Used for sliced sending request mode
	 * just for sending proccess in slice request mode
	 * @param req {const redis_request&} Request data object
	 *  request object
	 */
	const redis_result* run(dbuf_pool* pool, const redis_request& req,
		size_t nchildren, int* rw_timeout = NULL);

	//const redis_result* run(redis_command* cmd, size_t nchildren,
	//	int* rw_timeout = NULL);

protected:
	// Base class virtual function
	// @override
	bool open();

protected:
	socket_stream conn_;
	bool   check_addr_;
	char*  addr_;
	char*  pass_;
	bool   retry_;
	bool   authing_;
	bool   slice_req_;
	bool   slice_res_;
	int    dbnum_;
	sslbase_conf* ssl_conf_;

public:
	redis_result* get_objects(socket_stream& conn,
	 	dbuf_pool* pool, size_t nobjs);
	redis_result* get_object(socket_stream& conn, dbuf_pool* pool);
	redis_result* get_error(socket_stream& conn, dbuf_pool* pool);
	redis_result* get_status(socket_stream& conn, dbuf_pool* pool);
	redis_result* get_integer(socket_stream& conn, dbuf_pool* pool);
	redis_result* get_string(socket_stream& conn, dbuf_pool* pool);
	redis_result* get_array(socket_stream& conn, dbuf_pool* pool);

private:
	void put_data(dbuf_pool* pool, redis_result* rr,
		const char* data, size_t len);
	bool check_connection(const socket_stream& conn) const;
};

} // end namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

