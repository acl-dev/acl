#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/stream/sslbase_conf.hpp"
#include "acl_cpp/stream/sslbase_io.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_connection.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#endif
#include "redis_request.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

#define INT_LEN	11

redis_client::redis_client(const char* addr, int conn_timeout /* = 60 */,
	int rw_timeout /* = 30 */, bool retry /* = true */)
: check_addr_(false)
, retry_(retry)
, authing_(false)
, slice_req_(false)
, slice_res_(false)
, dbnum_(0)
, ssl_conf_(NULL)
{
	addr_ = acl_mystrdup(addr);
	pass_ = NULL;
	set_timeout(conn_timeout, rw_timeout);
}

redis_client::~redis_client(void)
{
	acl_myfree(addr_);
	if (pass_)
		acl_myfree(pass_);

	if (conn_.opened())
		conn_.close();
}

void redis_client::set_check_addr(bool on)
{
	check_addr_ = on;
}

void redis_client::set_ssl_conf(sslbase_conf* ssl_conf)
{
	ssl_conf_ = ssl_conf;
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

void redis_client::set_db(int dbnum)
{
	if (dbnum > 0)
		dbnum_ = dbnum;
}

socket_stream* redis_client::get_stream(bool auto_connect /* true */)
{
	if (conn_.opened())
		return (socket_stream*) &conn_;
	else if (!auto_connect)
		return NULL;
	else if (open())
		return (socket_stream*) &conn_;
	else
		return NULL;
}

bool redis_client::check_connection(socket_stream& conn)
{
	char peer[64];
	ACL_SOCKET fd = conn.sock_handle();

	if (acl_getpeername(fd, peer, sizeof(peer) - 1) == -1) {
		logger_error("getpeername failed: %s, fd: %d, addr: %s",
			last_serror(), (int) fd, addr_);
		return false;
	}

	if (strcmp(peer, addr_) != 0) {
		logger_error("addr no matched, peer: %s, addr: %s, fd: %d",
			peer, addr_, (int) fd);
		return false;
	}

	return true;
}

bool redis_client::open(void)
{
	if (conn_.opened())
		return true;
	if (conn_.open(addr_, conn_timeout_, rw_timeout_) == false) {
		logger_error("connect redis %s error: %s",
			addr_, last_serror());
		return false;
	}

	// 如果 SSL 配置项非空，则自动进行 SSL 握手
	if (ssl_conf_) {
		sslbase_io* ssl = ssl_conf_->create(false);
		if (conn_.setup_hook(ssl) == ssl) {
			logger_error("open ssl failed, addr=%s", addr_);
			ssl->destroy();
			conn_.close();
			return false;
		}
	}

	// 如果连接密码非空，则尝试用该密码向 redis-server 认证合法性
	if (pass_ && *pass_) { // && !authing_)
		// 设置当前连接的状态为认证状态，以避免进入死循环
		authing_ = true;
		redis_connection connection(this);
		if (connection.auth(pass_) == false) {
			authing_ = false;
			conn_.close();
			logger_error("auth error, addr: %s, passwd: %s",
				addr_, pass_);
			return false;
		}
		authing_ = false;
	}

	if (dbnum_ > 0) {
		redis_connection connection(this);
		if (connection.select(dbnum_) == false) {
			conn_.close();
			logger_error("select db error, db=%d", dbnum_);
			return false;
		}
	}
	return true;
}

void redis_client::close(void)
{
	if (conn_.opened())
		conn_.close();
}

bool redis_client::eof(void) const
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

void redis_client::put_data(dbuf_pool* dbuf, redis_result* rr,
	const char* data, size_t len)
{
	char* buf = (char*) dbuf->dbuf_alloc(len + 1);
	if (len > 0)
		memcpy(buf, data, len);
	buf[len] = 0;
	rr->put(buf, len);
}

redis_result* redis_client::get_error(socket_stream& conn, dbuf_pool* dbuf)
{
	string& buf = conn.get_buf();
	buf.clear();
	if (conn_.gets(buf) == false) {
		logger_error("gets error, server: %s", conn.get_peer(true));
		return NULL;
	}

	redis_result* rr = new(dbuf) redis_result(dbuf);
	rr->set_type(REDIS_RESULT_ERROR);
	rr->set_size(1);

	put_data(dbuf, rr, buf.c_str(), buf.length());
	return rr;
}

redis_result* redis_client::get_status(socket_stream& conn, dbuf_pool* dbuf)
{
	string& buf = conn.get_buf();
	buf.clear();
	if (conn_.gets(buf) == false) {
		logger_error("gets error, server: %s", conn.get_peer(true));
		return NULL;
	}

	redis_result* rr = new(dbuf) redis_result(dbuf);
	rr->set_type(REDIS_RESULT_STATUS);
	rr->set_size(1);

	put_data(dbuf, rr, buf.c_str(), buf.length());
	return rr;
}

redis_result* redis_client::get_integer(socket_stream& conn, dbuf_pool* dbuf)
{
	string& buf = conn.get_buf();
	buf.clear();
	if (conn_.gets(buf) == false) {
		logger_error("gets error, server: %s", conn.get_peer(true));
		return NULL;
	}

	redis_result* rr = new(dbuf) redis_result(dbuf);
	rr->set_type(REDIS_RESULT_INTEGER);
	rr->set_size(1);

	put_data(dbuf, rr, buf.c_str(), buf.length());
	return rr;
}

redis_result* redis_client::get_string(socket_stream& conn, dbuf_pool* dbuf)
{
	string& sbuf = conn.get_buf();
	sbuf.clear();
	if (conn_.gets(sbuf) == false) {
		logger_error("gets error, server: %s", conn.get_peer(true));
		return NULL;
	}
	redis_result* rr = new(dbuf) redis_result(dbuf);
	rr->set_type(REDIS_RESULT_STRING);
	int len = atoi(sbuf.c_str());
	if (len < 0)
		return rr;

	char* buf;

	if (!slice_res_) {
		rr->set_size(1);
		buf = (char*) dbuf->dbuf_alloc(len + 1);
		if (len > 0 && conn_.read(buf, (size_t) len) == -1) {
			logger_error("read error, server: %s",
				conn.get_peer(true));
			return NULL;
		}
		buf[len] = 0;
		rr->put(buf, (size_t) len);

		// 读 \r\n
		sbuf.clear();
		if (conn_.gets(sbuf) == false) {
			logger_error("gets error, server: %s",
				conn.get_peer(true));
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

	while (len > 0) {
		n = len > CHUNK_LENGTH - 1 ? CHUNK_LENGTH - 1 : len;
		buf = (char*) dbuf->dbuf_alloc((size_t) (n + 1));
		if (conn_.read(buf, (size_t) n) == -1) {
			logger_error("read data error, server: %s",
				conn.get_peer(true));
			return NULL;
		}
		buf[n] = 0;
		rr->put(buf, (size_t) n);
		len -= n;
	}
	
	sbuf.clear();
	if (conn_.gets(sbuf) == false) {
		logger_error("gets error, server: %s", conn.get_peer(true));
		return NULL;
	}
	return rr;
}

redis_result* redis_client::get_array(socket_stream& conn, dbuf_pool* dbuf)
{
	string& buf = conn.get_buf();
	buf.clear();
	if (conn.gets(buf) == false) {
		logger_error("gets error, server: %s", conn.get_peer(true));
		return NULL;
	}

	redis_result* rr = new(dbuf) redis_result(dbuf);
	rr->set_type(REDIS_RESULT_ARRAY);
	int count = atoi(buf.c_str());
	if (count <= 0)
		return rr;

	rr->set_size((size_t) count);

	for (int i = 0; i < count; i++) {
		redis_result* child = get_object(conn, dbuf);
		if (child == NULL)
			return NULL;
		rr->put(child, i);
	}

	return rr;
}

redis_result* redis_client::get_object(socket_stream& conn, dbuf_pool* dbuf)
{
	char ch;
	if (conn.read(ch) == false) {
		logger_warn("read char error: %s, server: %s, fd: %u",
			last_serror(), conn.get_peer(true),
			(unsigned) conn.sock_handle());
		return NULL;
	}

	switch (ch) {
	case '-':	// ERROR
		return get_error(conn, dbuf);
	case '+':	// STATUS
		return get_status(conn, dbuf);
	case ':':	// INTEGER
		return get_integer(conn, dbuf);
	case '$':	// STRING
		return get_string(conn, dbuf);
	case '*':	// ARRAY
		return get_array(conn, dbuf);
	default:	// INVALID
		logger_error("invalid first char: %c, %d", ch, ch);
		return NULL;
	}
}

redis_result* redis_client::get_objects(socket_stream& conn,
	dbuf_pool* dbuf, size_t nobjs)
{
	acl_assert(nobjs >= 1);

	redis_result* objs = new(dbuf) redis_result(dbuf);
	objs->set_type(REDIS_RESULT_ARRAY);
	objs->set_size(nobjs);

	for (size_t i = 0; i < nobjs; i++) {
		redis_result* obj = get_object(conn, dbuf);
		if (obj == NULL)
			return NULL;
		objs->put(obj, i);
	}
	return objs;
}

const redis_result* redis_client::run(dbuf_pool* dbuf, const string& req,
	size_t nchildren, int* rw_timeout /* = NULL */)
{
	// 重置协议处理状态
	bool retried = false;
	redis_result* result;

	while (true) {
		if (open() == false) {
			logger_error("open error: %s, addr: %s, req: %s",
				last_serror(), addr_, req.c_str());
			return NULL;
		}

		if (rw_timeout != NULL)
			conn_.set_rw_timeout(*rw_timeout);

		if (check_addr_ && check_connection(conn_) == false) {
			logger_error("CHECK_CONNECTION FAILED!");
			close();
			break;
		}

		if (!req.empty() && conn_.write(req) == -1) {
			close();

			if (retry_ && !retried) {
				retried = true;
				continue;
			}

			logger_error("write to redis(%s) error: %s, req: %s",
				addr_, last_serror(), req.c_str());
			return NULL;
		}
		if (nchildren >= 1)
			result = get_objects(conn_, dbuf, nchildren);
		else
			result = get_object(conn_, dbuf);
		if (result != NULL) {
			if (rw_timeout != NULL)
				conn_.set_rw_timeout(rw_timeout_);
			return result;
		}
#if defined(_WIN32) || defined(_WIN64)
		int error = last_error();
#endif
		close();
#if defined(_WIN32) || defined(_WIN64)
		set_error(error);
#endif
		if (req.empty()) {
			logger_error("no retry for request is empty");
			break;
		}

		if (!retry_ || retried) {
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

const redis_result* redis_client::run(dbuf_pool* dbuf, const redis_request& req,
	size_t nchildren, int* rw_timeout /* = NULL */)
{
	// 重置协议处理状态
	bool retried = false;

	redis_result* result;

	struct iovec* iov = req.get_iovec();
	size_t size = req.get_size();

	while (true) {
		if (open() == false)
			return NULL;

		if (rw_timeout != NULL)
			conn_.set_rw_timeout(*rw_timeout);

		if (check_addr_ && check_connection(conn_) == false) {
			logger_error("CHECK_CONNECTION FAILED!");
			close();
			break;
		}

		if (size > 0 && conn_.writev(iov, (int) size) == -1) {
			close();

			if (retry_ && !retried) {
				retried = true;
				continue;
			}

			logger_error("write to redis(%s) error: %s",
				addr_, last_serror());
			return NULL;
		}

		if (nchildren >= 1)
			result = get_objects(conn_, dbuf, nchildren);
		else
			result = get_object(conn_, dbuf);

		if (result != NULL) {
			if (rw_timeout != NULL)
				conn_.set_rw_timeout(rw_timeout_);
			return result;
		}

		close();

		if (!retry_ || retried || size == 0) {
			logger_error("retry_: %s, retried: %s, size: %d",
				retry_ ? "yes" : "no", retried ? "yes" : "no",
				(int) size);
			break;
		}

		retried = true;
	}

	return NULL;
}

} // end namespace acl

#endif // ACL_CLIENT_ONLY
