#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "../stream/socket_stream.hpp"
#include "../connpool/connect_client.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

class string;
class dbuf_pool;
class redis_result;
class redis_request;
class redis_command;
class sslbase_conf;

/**
 * redis 客户端对象网络通信类，通过此类将组织好的 redis 请求命令发给 redis
 * 服务端，同时接收 redis 服务端响应结果；该类继承于 connect_client 类，主要
 * 为了使用连接池功能。
 * redis client network IO class. The redis request is sent to server
 * and the server's respond is handled in this class. The class inherits
 * connect_client, which can use the connection pool function.
 */
class ACL_CPP_API redis_client : public connect_client
{
public:
	/**
	 * 构造函数
	 * constructor
	 * @param addr {const char*} redis-server 监听地址
	 *  the redis-server listening addr
	 * @param conn_timeout {int} 连接 redis-server 的超时时间(秒)
	 *  the timeout in seconds to connect the redis-server
	 * @param rw_timeout {int} 与 redis-server 进行通信的 IO 超时时间(秒)
	 *  the network IO timeout in seconds with the redis-server
	 */
	redis_client(const char* addr, int conn_timeout = 60,
		int rw_timeout = 30, bool retry = true);
	virtual ~redis_client(void);

	/**
	 * 设置 SSL 通信方式下的配置句柄，内部缺省值为 NULL，如果设置了 SSL 连
	 * 接配置对象，则内部切换成 SSL 通信方式
	 * set SSL communication with Redis-server if ssl_conf not NULL
	 * @param ssl_conf {sslbase_conf*}
	 */
	void set_ssl_conf(sslbase_conf* ssl_conf);

	/**
	 * 调用本函数设置连接 redis 服务的连接密码
	 * @param pass {const char*}
	 */
	void set_password(const char* pass);

	/**
	 * 设置本连接所对应的 db，当连接建立后如果指定的 db 值大于 0，则内部自动
	 * 选择对应的 db，注意：该功能仅针对非集群模式
	 * if db > 0 in no cluster mode, select the db when the connection
	 * is created.
	 * @param dbnum {int}
	 */
	void set_db(int dbnum);

	/**
	 * 获得本连接所选择的 db
	 * get db for the connection
	 * @return {int}
	 */
	int get_db(void) const
	{
		return dbnum_;
	}

	/**
	 * 获得当前连接的服务器地址，即由 redis_client 构造时传入的地址
	 * @return {const char*}
	 */
	const char* get_addr(void) const
	{
		return addr_;
	}

	/**
	 * 在进行每个命令处理前，是否要求检查 socket 句柄与地址的匹配情况，当
	 * 打开该选项时，将会严格检查匹配情况，但会影响一定性能，因此该设置仅
	 * 用在 DEBUG 时的运行场景
	 * @param on {bool}
	 */
	void set_check_addr(bool on);

	/**
	 * 判断该网络连接对象是否已经关闭
	 * check if the connection has been finish
	 * @return {bool}
	 */
	bool eof(void) const;

	/**
	 * 关闭网络连接
	 * close the connection to the redis-server
	 */
	void close(void);

	/**
	 * 获得网络连接流，当连接关闭时内部会自动重连一次
	 * get acl::socket_stream from the connection
	 * @param auto_connect {bool} 内部是否需要自动连接服务端
	 *  if we should connect the redis server automaticlly
	 * @return {acl::socket_stream*} 如果连接已经关闭则返回 NULL
	 *  NULL will be returned if the connectioin has been closed
	 */
	socket_stream* get_stream(bool auto_connect = true);

	/**
	 * 对于请求数据包，此函数设置在组装请求数据包的时候合成一个数据包发送
	 * just for request package, setting flag for sending data with
	 * multi data chunks; this is useful when the request data is large
	 * @param on {bool} 当为 true 时则不会将请求数据合成一个数据包发送
	 *  if true the request data will not be combined one package
	 */
	void set_slice_request(bool on);

	/**
	 * 对于响应数据包，此函数设置是否将 redis-server 响应的数据分拆成多个数据块，
	 * 这对于大的数据包有用处，可以不必一次性分配一个连续性的大内存
	 * just for response package, settint flag for receiving data
	 * if split the large response data into multi little chunks
	 * @param on {bool} 当为 true 时则对响应数据包进行拆分
	 *  if true the response data will be splitted into multi little
	 *  data, which is useful for large reponse data for avoiding
	 *  malloc large continuously memory from system.
	 */
	void set_slice_respond(bool on);

	/**
	 * 用于非分片发送方式，向 redis-server 发送请求数据，同时读取并分析
	 * 服务端返回的响应数据
	 * send request to redis-server, and read/anlyse response from server,
	 * this function will be used for no-slice request mode.
	 * @param pool {dbuf_pool*} 内存池管理器对象
	 *  memory pool manager
	 * @param req {const string&} 请求数据包
	 *  the request package
	 * @param nchildren {size_t} 响应数据有几个数据对象
	 *  the data object number in the server's response data
	 * @return {const redis_result*} 读到的服务器响应对象，返回 NULL 则出错,
	 *  该对象不必手工释放，因为其是在 pool 内存池对象上动态分配的，所以当
	 *  释放 pool 时该结果对象一同被释放
	 *  the result object from server's response, NULL will be returned
	 *  when some error happens; the result object needn't be freed
	 *  manually, which was created in the pool object, and will be freed
	 *  when the pool were freed.
	 *  
	 */
	const redis_result* run(dbuf_pool* pool, const string& req,
		size_t nchildren, int* rw_timeout = NULL);

	/**
	 * 用于分片发送请求方式
	 * just for sending proccess in slice request mode
	 * @param req {const redis_request&} 请求数据对象
	 *  request object
	 */
	const redis_result* run(dbuf_pool* pool, const redis_request& req,
		size_t nchildren, int* rw_timeout = NULL);

	const redis_result* run(redis_command* cmd, size_t nchildren,
		int* rw_timeout = NULL);

protected:
	// 基类虚函数
	// @override
	bool open(void);

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
	bool check_connection(socket_stream& conn);
};

} // end namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
