#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/connpool/connect_client.hpp"

namespace acl
{

class dbuf_pool;
class redis_result;
class redis_request;

/**
 * redis 客户端对象网络通信类，通过此类将组织好的 redis 请求命令发给 redis 服务端，
 * 同时接收 redis 服务端响应结果；该类继承于 connect_client 类，主要为了使用连接池
 * 功能。
 * redis client network IO class. The redis request is sent to server
 * and the server's respond is handled in this class. The class inherits
 * connect_client, which can use the connection pool function.
 */
class ACL_CPP_API redis_client : public connect_client
{
public:
	redis_client(const char* addr, int conn_timeout = 60,
		int rw_timeout = 30, bool retry = true);
	~redis_client();

	bool eof() const;
	void close();

	socket_stream* get_stream();

	void set_slice_request(bool on);
	void set_slice_respond(bool on);

	const redis_result* run(dbuf_pool* pool, const string& req,
		size_t nchildren);
	const redis_result* run(dbuf_pool* pool, const redis_request& req,
		size_t nchildren);

protected:
	// 基类虚函数
	virtual bool open();

private:
	socket_stream conn_;
	char* addr_;
	int   conn_timeout_;
	int   rw_timeout_;
	bool  retry_;
	string  buf_;
	bool slice_req_;
	bool slice_res_;

	redis_result* get_redis_objects(dbuf_pool* pool, size_t nobjs);
	redis_result* get_redis_object(dbuf_pool* pool);
	redis_result* get_redis_error(dbuf_pool* pool);
	redis_result* get_redis_status(dbuf_pool* pool);
	redis_result* get_redis_integer(dbuf_pool* pool);
	redis_result* get_redis_string(dbuf_pool* pool);
	redis_result* get_redis_array(dbuf_pool* pool);

	void put_data(dbuf_pool* pool, redis_result* rr,
		const char* data, size_t len);
};

} // end namespace acl
