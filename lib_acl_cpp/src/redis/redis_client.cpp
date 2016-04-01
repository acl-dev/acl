#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_connection.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#endif
#include "redis_request.hpp"

namespace acl
{

#define INT_LEN	11

redis_client::redis_client(const char* addr, int conn_timeout /* = 60 */,
	int rw_timeout /* = 30 */, bool retry /* = true */)
: retry_(retry)
, slice_req_(false)
, slice_res_(false)
{
	addr_ = acl_mystrdup(addr);
	pass_ = NULL;
	set_timeout(conn_timeout, rw_timeout);
}

redis_client::~redis_client()
{
	acl_myfree(addr_);
	if (pass_)
		acl_myfree(pass_);

	if (conn_.opened())
		conn_.close();
}

void redis_client::set_password(const char* pass)
{
	if (pass_)
		acl_myfree(pass_);
	if (pass && *pass)
		pass_ = acl_mystrdup(pass);
	else
		pass_ = NULL;
}

socket_stream* redis_client::get_stream()
{
	if (conn_.opened())
		return (socket_stream*) &conn_;
	else if (open())
		return (socket_stream*) &conn_;
	else
		return NULL;
}

bool redis_client::open()
{
	if (conn_.opened())
		return true;
	if (conn_.open(addr_, conn_timeout_, rw_timeout_) == false)
	{
		logger_error("connect redis %s error: %s",
			addr_, last_serror());
		return false;
	}

	// 如果连接密码非空，则尝试用该密码向 redis-server 认证合法性
	if (pass_ && *pass_)
	{
		redis_connection connection(this);
		if (connection.auth(pass_) == false)
		{
			logger_error("auth error, addr: %s, passwd: %s",
				addr_, pass_);
			// 此处返回 true，以便于上层使用该连接访问时
			// 由 redis-server 直接给命令报未认证的错训，
			// 从而免得在 redis_command 类中不断地重试连接
			return true;
		}
	}

	return true;
}

void redis_client::close()
{
	if (conn_.opened())
		conn_.close();
}

bool redis_client::eof() const
{
	return conn_.eof();
}

void redis_client::set_slice_request(bool on)
{
	slice_req_ = on;
}

void redis_client::set_slice_respond(bool on)
{
	slice_res_ = on;
}

/////////////////////////////////////////////////////////////////////////////

void redis_client::put_data(dbuf_pool* pool, redis_result* rr,
	const char* data, size_t len)
{
	char* buf = (char*) pool->dbuf_alloc(len + 1);
	if (len > 0)
		memcpy(buf, data, len);
	buf[len] = 0;
	rr->put(buf, len);
}

redis_result* redis_client::get_redis_error(dbuf_pool* pool)
{
	buf_.clear();
	if (conn_.gets(buf_) == false)
	{
		logger_error("gets line error, server: %s", addr_);
		return NULL;
	}

	redis_result* rr = new(pool) redis_result(pool);
	rr->set_type(REDIS_RESULT_ERROR);
	rr->set_size(1);

	put_data(pool, rr, buf_.c_str(), buf_.length());
	return rr;
}

redis_result* redis_client::get_redis_status(dbuf_pool* pool)
{
	buf_.clear();
	if (conn_.gets(buf_) == false)
	{
		logger_error("gets line error, server: %s", addr_);
		return NULL;
	}

	redis_result* rr = new(pool) redis_result(pool);
	rr->set_type(REDIS_RESULT_STATUS);
	rr->set_size(1);

	put_data(pool, rr, buf_.c_str(), buf_.length());
	return rr;
}

redis_result* redis_client::get_redis_integer(dbuf_pool* pool)
{
	buf_.clear();
	if (conn_.gets(buf_) == false)
	{
		logger_error("gets line error, server: %s", addr_);
		return NULL;
	}

	redis_result* rr = new(pool) redis_result(pool);
	rr->set_type(REDIS_RESULT_INTEGER);
	rr->set_size(1);

	put_data(pool, rr, buf_.c_str(), buf_.length());
	return rr;
}

redis_result* redis_client::get_redis_string(dbuf_pool* pool)
{
	buf_.clear();
	if (conn_.gets(buf_) == false)
	{
		logger_error("gets line error, server: %s", addr_);
		return NULL;
	}
	redis_result* rr = new(pool) redis_result(pool);
	rr->set_type(REDIS_RESULT_STRING);
	int len = atoi(buf_.c_str());
	if (len < 0)
		return rr;

	char*  buf;

	if (!slice_res_)
	{
		rr->set_size(1);
		buf = (char*) pool->dbuf_alloc(len + 1);
		if (len > 0 && conn_.read(buf, (size_t) len) == -1)
		{
			logger_error("read data error, server: %s", addr_);
			return NULL;
		}
		buf[len] = 0;
		rr->put(buf, (size_t) len);

		// 读 \r\n
		buf_.clear();
		if (conn_.gets(buf_) == false)
		{
			logger_error("gets line error, server: %s", addr_);
			return NULL;
		}
		return rr;
	}

	// 将可能返回的大内存分成不连接的内存链存储

#define CHUNK_LENGTH	8192

	size_t size = (size_t) len / CHUNK_LENGTH;
	if ((size_t) len % CHUNK_LENGTH != 0)
		size++;
	rr->set_size(size);
	int    n;

	while (len > 0)
	{
		n = len > CHUNK_LENGTH - 1 ? CHUNK_LENGTH - 1 : len;
		buf = (char*) pool->dbuf_alloc((size_t) (n + 1));
		if (conn_.read(buf, (size_t) n) == -1)
		{
			logger_error("read data error, server: %s", addr_);
			return NULL;
		}
		buf[n] = 0;
		rr->put(buf, (size_t) n);
		len -= n;
	}
	
	buf_.clear();
	if (conn_.gets(buf_) == false)
	{
		logger_error("gets line error, server: %s", addr_);
		return NULL;
	}
	return rr;
}

redis_result* redis_client::get_redis_array(dbuf_pool* pool)
{
	buf_.clear();
	if (conn_.gets(buf_) == false)
		return NULL;

	redis_result* rr = new(pool) redis_result(pool);
	rr->set_type(REDIS_RESULT_ARRAY);
	int count = atoi(buf_.c_str());
	if (count <= 0)
		return rr;

	rr->set_size((size_t) count);

	for (int i = 0; i < count; i++)
	{
		redis_result* child = get_redis_object(pool);
		if (child == NULL)
			return NULL;
		rr->put(child, i);
	}

	return rr;
}

redis_result* redis_client::get_redis_object(dbuf_pool* pool)
{
	char ch;
	if (conn_.read(ch) == false)
	{
		logger_warn("read char error: %s, server: %s, fd: %u",
			last_serror(), addr_, (unsigned) conn_.sock_handle());
		return NULL;
	}

	switch (ch)
	{
	case '-':	// ERROR
		return get_redis_error(pool);
	case '+':	// STATUS
		return get_redis_status(pool);
	case ':':	// INTEGER
		return get_redis_integer(pool);
	case '$':	// STRING
		return get_redis_string(pool);
	case '*':	// ARRAY
		return get_redis_array(pool);
	default:	// INVALID
		logger_error("invalid first char: %c, %d", ch, ch);
		return NULL;
	}
}

redis_result* redis_client::get_redis_objects(dbuf_pool* pool, size_t nobjs)
{
	acl_assert(nobjs >= 1);

	redis_result* objs = new(pool) redis_result(pool);
	objs->set_type(REDIS_RESULT_ARRAY);
	objs->set_size(nobjs);

	for (size_t i = 0; i < nobjs; i++)
	{
		redis_result* obj = get_redis_object(pool);
		if (obj == NULL)
			return NULL;
		objs->put(obj, i);
	}
	return objs;
}

const redis_result* redis_client::run(dbuf_pool* pool, const string& req,
	size_t nchildren)
{
	// 重置协议处理状态
	bool retried = false;
	redis_result* result;

	while (true)
	{
		if (open() == false)
		{
			logger_error("open error: %s, addr: %s, req: %s",
				last_serror(), addr_, req.c_str());
			return NULL;
		}

		if (!req.empty() && conn_.write(req) == -1)
		{
			close();

			if (retry_ && !retried)
			{
				retried = true;
				continue;
			}

			logger_error("write to redis(%s) error: %s, req: %s",
				addr_, last_serror(), req.c_str());
			return NULL;
		}

		if (nchildren >= 1)
			result = get_redis_objects(pool, nchildren);
		else
			result = get_redis_object(pool);

		if (result != NULL)
			return result;

		close();

		if (!retry_ || retried)
		{
			logger_error("result NULL, addr: %s, retry: %s, "
				"retried: %s, req: %s", addr_,
				retry_ ? "true" : "false",
				retried ? "true" : "false", req.c_str());

			break;
		}

		logger_error("result NULL, addr: %s, retry: %s, "
			"retried: %s, req: %s", addr_,
			retry_ ? "true" : "false",
			retried ? "true" : "false", req.c_str());

		retried = true;
	}

	return NULL;
}

const redis_result* redis_client::run(dbuf_pool* pool, const redis_request& req,
	size_t nchildren)
{
	// 重置协议处理状态
	bool retried = false;

	redis_result* result;

	struct iovec* iov = req.get_iovec();
	size_t size = req.get_size();

	while (true)
	{
		if (open() == false)
			return NULL;

		if (size > 0 && conn_.writev(iov, (int) size) == -1)
		{
			close();

			if (retry_ && !retried)
			{
				retried = true;
				continue;
			}

			logger_error("write to redis(%s) error: %s",
				addr_, last_serror());
			return NULL;
		}

		if (nchildren >= 1)
			result = get_redis_objects(pool, nchildren);
		else
			result = get_redis_object(pool);

		if (result != NULL)
			return result;

		close();

		if (!retry_ || retried)
			break;

		retried = true;
	}

	return NULL;
}

} // end namespace acl
