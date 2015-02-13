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
	// »ùÀàÐéº¯Êý
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
