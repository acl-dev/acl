#include "stdafx.h"
#include "redis_object.h"
#include "redis_client.h"

redis_client::redis_client(acl::socket_stream& conn)
: conn_(conn)
, ssl_conf_(NULL)
, buff_(NULL)
, tokens_(NULL)
{
}

redis_client::~redis_client(void)
{
	delete buff_;
}

void redis_client::set_ssl_conf(acl::sslbase_conf* ssl_conf)
{
	ssl_conf_ = ssl_conf;
}

bool redis_client::eof(void) const
{
	return conn_.eof();
}

/////////////////////////////////////////////////////////////////////////////

void redis_client::put_data(acl::dbuf_pool* dbuf, redis_object* obj,
	const char* data, size_t len)
{
	char* buf = (char*) dbuf->dbuf_alloc(len + 1);
	if (len > 0) {
		memcpy(buf, data, len);
	}
	buf[len] = 0;
	obj->put(buf, len);
}

redis_object* redis_client::get_error(acl::socket_stream& conn,
	acl::dbuf_pool* dbuf)
{
	acl::string& buf = conn.get_buf();
	buf.clear();
	if (conn_.gets(buf) == false) {
		logger_error("gets error, server: %s", conn.get_peer(true));
		return NULL;
	}

	redis_object* obj = new(dbuf) redis_object(dbuf);
	obj->set_type(REDIS_OBJECT_ERROR);
	obj->set_size(1);

	put_data(dbuf, obj, buf.c_str(), buf.length());
	return obj;
}

redis_object* redis_client::get_status(acl::socket_stream& conn,
	acl::dbuf_pool* dbuf)
{
	acl::string& buf = conn.get_buf();
	buf.clear();
	if (conn_.gets(buf) == false) {
		logger_error("gets error, server: %s", conn.get_peer(true));
		return NULL;
	}

	redis_object* obj = new(dbuf) redis_object(dbuf);
	obj->set_type(REDIS_OBJECT_STATUS);
	obj->set_size(1);

	put_data(dbuf, obj, buf.c_str(), buf.length());
	return obj;
}

redis_object* redis_client::get_integer(acl::socket_stream& conn,
	acl::dbuf_pool* dbuf)
{
	acl::string& buf = conn.get_buf();
	buf.clear();
	if (conn_.gets(buf) == false) {
		logger_error("gets error, server: %s", conn.get_peer(true));
		return NULL;
	}

	redis_object* obj = new(dbuf) redis_object(dbuf);
	obj->set_type(REDIS_OBJECT_INTEGER);
	obj->set_size(1);

	put_data(dbuf, obj, buf.c_str(), buf.length());
	return obj;
}

redis_object* redis_client::get_string(acl::socket_stream& conn,
	acl::dbuf_pool* dbuf)
{
	acl::string& sbuf = conn.get_buf();
	sbuf.clear();
	if (conn_.gets(sbuf) == false) {
		logger_error("gets error, server: %s", conn.get_peer(true));
		return NULL;
	}
	redis_object* obj = new(dbuf) redis_object(dbuf);
	obj->set_type(REDIS_OBJECT_STRING);
	int len = atoi(sbuf.c_str());
	if (len < 0) {
		return obj;
	}

	char* buf;

	obj->set_size(1);
	buf = (char*) dbuf->dbuf_alloc(len + 1);
	if (len > 0 && conn_.read(buf, (size_t) len) == -1) {
		logger_error("read error, server: %s",
				conn.get_peer(true));
		return NULL;
	}
	buf[len] = 0;
	obj->put(buf, (size_t) len);

	// 读 \r\n
	sbuf.clear();
	if (conn_.gets(sbuf) == false) {
		logger_error("gets error, server: %s", conn.get_peer(true));
		return NULL;
	}
	return obj;
}

redis_object* redis_client::get_array(acl::socket_stream& conn,
	acl::dbuf_pool* dbuf)
{
	acl::string& buf = conn.get_buf();
	buf.clear();
	if (conn.gets(buf) == false) {
		logger_error("gets error, server: %s", conn.get_peer(true));
		return NULL;
	}

	redis_object* obj = new(dbuf) redis_object(dbuf);
	obj->set_type(REDIS_OBJECT_ARRAY);
	int count = atoi(buf.c_str());
	if (count <= 0) {
		return obj;
	}

	obj->set_size((size_t) count);

	for (int i = 0; i < count; i++) {
		redis_object* child = get_object(conn, dbuf);
		if (child == NULL) {
			return NULL;
		}
		obj->put(child, i);
	}

	return obj;
}

redis_object* redis_client::get_line(acl::socket_stream& conn,
	acl::dbuf_pool* dbuf)
{
	if (buff_ == NULL) {
		buff_ = new acl::string(256);
	} else {
		buff_->clear();
	}

	if (!conn.gets(*buff_)) {
		logger_error("gets line from client error!");
		return NULL;
	}

	if (buff_->begin_with("quit", false)) {
		conn.format("+OK\r\n");
		return NULL;
	}

	tokens_ = &(buff_->split2(" \t"));

	redis_object* obj = new(dbuf) redis_object(dbuf);
	obj->set_type(REDIS_OBJECT_ARRAY);
	obj->set_size(tokens_->size());

	size_t i = 0;
	for (std::vector<acl::string>::const_iterator cit = tokens_->begin();
		cit != tokens_->end(); ++cit) {

		redis_object* child = new(dbuf) redis_object(dbuf);
		child->set_type(REDIS_OBJECT_STRING);
		child->set_size(1);
		child->put((*cit).c_str(), (*cit).size());
		obj->put(child, i++);
	}

	//printf(">>>Obj type=%d, size=%zd\n", (int) obj->get_type(),
	//		obj->get_size());
	return obj;
}

redis_object* redis_client::get_object(acl::socket_stream& conn,
	acl::dbuf_pool* dbuf)
{
	char ch;
	if (conn.read(ch) == false) {
		/*
		logger_warn("read char error: %s, server: %s, fd: %u",
			acl::last_serror(), conn.get_peer(true),
			(unsigned) conn.sock_handle());
		*/
		return NULL;
	}

	//printf(">>>>in get_object ch=%c\n", ch);

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
		conn.ugetch(ch);
		return get_line(conn, dbuf);
	}
}

redis_object* redis_client::get_objects(acl::socket_stream& conn,
	acl::dbuf_pool* dbuf, size_t nobjs)
{
	assert(nobjs >= 1);

	redis_object* objs = new(dbuf) redis_object(dbuf);
	objs->set_type(REDIS_OBJECT_ARRAY);
	objs->set_size(nobjs);

	for (size_t i = 0; i < nobjs; i++) {
		redis_object* obj = get_object(conn, dbuf);
		if (obj == NULL) {
			return NULL;
		}
		objs->put(obj, i);
	}
	return objs;
}

const redis_object* redis_client::read_reply(acl::dbuf_pool& dbuf,
	size_t nchildren /* = 0 */, int* rw_timeout /* = NULL */)
{
	// 重置协议处理状态
	redis_object* obj;

	while (true) {
		if (nchildren >= 1) {
			obj = get_objects(conn_, &dbuf, nchildren);
		} else {
			obj = get_object(conn_, &dbuf);
		}
		if (obj != NULL) {
			if (rw_timeout != NULL) {
				conn_.set_rw_timeout(*rw_timeout);
			}
			return obj;
		}
	}

	return NULL;
}

bool redis_client::read_request(acl::dbuf_pool& dbuf,
	std::vector<const redis_object*>& out)
{
	while (true) {
		redis_object* obj = get_object(conn_, &dbuf);
		if (obj == NULL) {
			return false;
		}

		out.push_back(obj);

		// Try to check if there're some data in the reading buffer.
		ACL_VSTREAM* fp = conn_.get_vstream();
		if (fp->read_cnt <= 0) {
			break;
		}
	}

	return true;

}
