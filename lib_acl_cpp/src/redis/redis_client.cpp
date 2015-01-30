#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "redis_request.hpp"

namespace acl
{

#define INT_LEN	11

redis_client::redis_client(const char* addr, int conn_timeout /* = 60 */,
	int rw_timeout /* = 30 */, bool retry /* = true */)
: slice_req_(false)
, slice_res_(false)
, used_(0)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
, retry_(retry)
, argv_size_(0)
, argv_(NULL)
, argv_lens_(NULL)
, result_(NULL)
{
	addr_ = acl_mystrdup(addr);
//	pool_ = NEW dbuf_pool(40960);
	pool_ = NEW dbuf_pool();
	req_ = NEW redis_request();
}

redis_client::~redis_client()
{
	acl_myfree(addr_);
	if (result_)
		result_->reset();
	delete pool_;
	if (argv_ != NULL)
		acl_myfree(argv_);
	if (argv_lens_ != NULL)
		acl_myfree(argv_lens_);
	if (conn_.opened())
		conn_.close();
	delete req_;
}

void redis_client::set_slice_request(bool on)
{
	slice_req_ = on;
}

void redis_client::set_slice_respond(bool on)
{
	slice_res_ = on;
}

void redis_client::reset()
{
	// 只有当本连接对象被重复使用时才需要状态重置
	if (used_ > 0)
	{
		if (result_)
			result_->reset();
		delete pool_;
		pool_ = NEW dbuf_pool();
	}
}

void redis_client::argv_space(size_t n)
{
	if (argv_size_ >= n)
		return;
	argv_size_ = n;
	if (argv_ == NULL)
	{
		argv_ = (const char**) acl_mymalloc(n * sizeof(char*));
		argv_lens_ = (size_t*) acl_mymalloc(n * sizeof(size_t));
	}
	else
	{
		argv_ = (const char**) acl_myrealloc(argv_, n * sizeof(char*));
		argv_lens_ = (size_t*) acl_myrealloc(argv_lens_,
				n * sizeof(size_t));
	}
}

socket_stream* redis_client::get_stream()
{
	if (conn_.opened())
		return &conn_;
	else if (open())
		return &conn_;
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

/////////////////////////////////////////////////////////////////////////////

void redis_client::put_data(redis_result* rr, const char* data, size_t len)
{
	char* buf = (char*) pool_->dbuf_alloc(len + 1);
	if (len > 0)
		memcpy(buf, data, len);
	buf[len] = 0;
	rr->put(buf, len);
}

redis_result* redis_client::get_redis_error()
{
	buf_.clear();
	if (conn_.gets(buf_) == false)
		return NULL;

	redis_result* rr = new(pool_) redis_result(pool_);
	rr->set_type(REDIS_RESULT_ERROR);
	rr->set_size(1);

	put_data(rr, buf_.c_str(), buf_.length());
	return rr;
}

redis_result* redis_client::get_redis_status()
{
	buf_.clear();
	if (conn_.gets(buf_) == false)
		return NULL;

	redis_result* rr = new(pool_) redis_result(pool_);
	rr->set_type(REDIS_RESULT_STATUS);
	rr->set_size(1);

	put_data(rr, buf_.c_str(), buf_.length());
	return rr;
}

redis_result* redis_client::get_redis_integer()
{
	buf_.clear();
	if (conn_.gets(buf_) == false)
		return NULL;

	redis_result* rr = new(pool_) redis_result(pool_);
	rr->set_type(REDIS_RESULT_INTEGER);
	rr->set_size(1);

	put_data(rr, buf_.c_str(), buf_.length());
	return rr;
}

redis_result* redis_client::get_redis_string()
{
	buf_.clear();
	if (conn_.gets(buf_) == false)
		return NULL;
	redis_result* rr = new(pool_) redis_result(pool_);
	rr->set_type(REDIS_RESULT_STRING);
	int len = atoi(buf_.c_str());
	if (len < 0)
		return rr;

	char*  buf;

	if (!slice_res_)
	{
		rr->set_size(1);
		buf = (char*) pool_->dbuf_alloc(len + 1);
		if (conn_.read(buf, (size_t) len) == -1)
			return NULL;
		buf[len] = 0;
		rr->put(buf, (size_t) len);

		// 读 \r\n
		buf_.clear();
		if (conn_.gets(buf_) == false)
			return NULL;
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
		buf = (char*) pool_->dbuf_alloc((size_t) (n + 1));
		if (conn_.read(buf, (size_t) n) == -1)
			return NULL;
		buf[n] = 0;
		rr->put(buf, (size_t) n);
		len -= n;
	}
	
	buf_.clear();
	if (conn_.gets(buf_) == false)
		return NULL;
	return rr;
}

redis_result* redis_client::get_redis_array()
{
	buf_.clear();
	if (conn_.gets(buf_) == false)
		return NULL;

	redis_result* rr = new(pool_) redis_result(pool_);
	rr->set_type(REDIS_RESULT_ARRAY);
	int count = atoi(buf_.c_str());
	if (count <= 0)
		return rr;

	rr->set_size((size_t) count);

	for (int i = 0; i < count; i++)
	{
		redis_result* child = get_redis_object();
		if (child == NULL)
			return NULL;
		rr->put(child, i);
	}

	return rr;
}

redis_result* redis_client::get_redis_object()
{
	char ch;
	if (conn_.read(ch) == false)
	{
		logger_error("read first char error");
		return NULL;
	}

	switch (ch)
	{
	case '-':	// ERROR
		return get_redis_error();
	case '+':	// STATUS
		return get_redis_status();
	case ':':	// INTEGER
		return get_redis_integer();
	case '$':	// STRING
		return get_redis_string();
	case '*':	// ARRAY
		return get_redis_array();
	default:	// INVALID
		logger_error("invalid first char: %c, %d", ch, ch);
		return NULL;
	}
}

redis_result* redis_client::get_redis_objects(size_t nobjs)
{
	acl_assert(nobjs >= 1);

	redis_result* objs = new(pool_) redis_result(pool_);
	objs->set_type(REDIS_RESULT_ARRAY);
	objs->set_size(nobjs);

	for (size_t i = 0; i < nobjs; i++)
	{
		redis_result* obj = get_redis_object();
		if (obj == NULL)
			return NULL;
		objs->put(obj, i);
	}
	return objs;
}

const redis_result* redis_client::run(size_t nchildren /* = 0 */)
{
	// 本连接使用次数递增
	used_++;

	if (slice_req_)
		return run2(nchildren);
	else
		return run1(nchildren);
}

const redis_result* redis_client::run1(size_t nchildren /* = 0 */)
{
	// 重置协议处理状态
	bool retried = false;

	while (true)
	{
		if (!conn_.opened() && conn_.open(addr_, conn_timeout_,
			rw_timeout_) == false)
		{
			logger_error("connect server: %s error: %s",
				addr_, last_serror());
			return NULL;
		}

		if (!request_.empty() && conn_.write(request_) == -1)
		{
			conn_.close();
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
			result_ = get_redis_objects(nchildren);
		else
			result_ = get_redis_object();

		if (result_ != NULL)
			return result_;
		conn_.close();

		if (!retry_ || retried)
			break;

		retried = true;
	}

	return NULL;

}

const redis_result* redis_client::run2(size_t nchildren /* = 0 */)
{
	// 重置协议处理状态
	bool retried = false;

	struct iovec* iov = req_->get_iovec();
	size_t size = req_->get_size();

	while (true)
	{
		if (!conn_.opened() && conn_.open(addr_, conn_timeout_,
			rw_timeout_) == false)
		{
			logger_error("connect server: %s error: %s",
				addr_, last_serror());
			return NULL;
		}

		if (size > 0 && conn_.writev(iov, (int) size) == -1)
		{
			conn_.close();
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
			result_ = get_redis_objects(nchildren);
		else
			result_ = get_redis_object();

		if (result_ != NULL)
			return result_;
		conn_.close();

		if (!retry_ || retried)
			break;

		retried = true;
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////

int redis_client::get_number(bool* success /* = NULL */)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_INTEGER)
	{
		if (success)
			*success = false;
		return -1;
	}
	if (success)
		*success = true;
	return result->get_integer();
}

long long int redis_client::get_number64(bool* success /* = NULL */)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_INTEGER)
	{
		if (success)
			*success = false;
		return -1;
	}
	if (success)
		*success = true;
	return result->get_integer64();
}

int redis_client::get_number(std::vector<int>& out)
{
	out.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0)
		return 0;
	out.reserve(size);

	const redis_result* rr;
	for (size_t i = 0; i < size; i++)
	{
		rr = children[i];
		out.push_back(rr->get_integer());
	}

	return size;
}

int redis_client::get_number64(std::vector<long long int>& out)
{
	out.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0)
		return 0;
	out.reserve(size);

	const redis_result* rr;
	for (size_t i = 0; i < size; i++)
	{
		rr = children[i];
		out.push_back(rr->get_integer64());
	}

	return size;
}

bool redis_client::get_status(const char* success /* = "OK" */)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_STATUS)
		return false;
	const char* status = result->get_status();
	if (status == NULL)
		return false;
	else if (success == NULL || strcasecmp(status, success) == 0)
		return true;
	else
		return false;
}

int redis_client::get_status(std::vector<bool>& out)
{
	out.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0)
		return 0;

	out.reserve(size);

	const redis_result* rr;
	for (size_t i = 0; i < size; i++)
	{
		rr = children[i];
		out.push_back(rr->get_integer() > 0 ? true : false);
	}

	return (int) size;
}

const char* redis_client::get_status_string()
{
	const redis_result* result = run();
	return result == NULL ? NULL : result->get_status();
}

int redis_client::get_string(string& buf)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_STRING)
		return -1;
	return result->argv_to_string(buf);
}

int redis_client::get_string(string* buf)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_STRING)
		return -1;
	if (buf == NULL)
		return (int) result->get_length();
	return result->argv_to_string(*buf);
}

int redis_client::get_string(char* buf, size_t size)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_STRING)
		return -1;
	return result->argv_to_string(buf, size);
}

int redis_client::get_strings(std::vector<string>& out)
{
	out.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL)
		return 0;

	if (size > 0)
		out.reserve(size);

	const redis_result* rr;
	string buf(4096);

	for (size_t i = 0; i < size; i++)
	{
		rr = children[i];
		if (rr == NULL || rr->get_type() != REDIS_RESULT_STRING)
			out.push_back("");
		else if (rr->get_size() == 0)
			out.push_back("");
		else 
		{
			rr->argv_to_string(buf);
			out.push_back(buf);
			buf.clear();
		}
	}

	return (int) size;
}

int redis_client::get_strings(std::vector<string>* out)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;
	if (out == NULL)
		return result->get_size();

	out->clear();

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL)
		return 0;

	if (size > 0)
		out->reserve(size);

	const redis_result* rr;
	string buf(4096);

	for (size_t i = 0; i < size; i++)
	{
		rr = children[i];
		if (rr == NULL || rr->get_type() != REDIS_RESULT_STRING)
			out->push_back("");
		else if (rr->get_size() == 0)
			out->push_back("");
		else 
		{
			rr->argv_to_string(buf);
			out->push_back(buf);
			buf.clear();
		}
	}

	return (int) size;
}

int redis_client::get_strings(std::map<string, string>& out)
{
	out.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;
	if (result->get_size() == 0)
		return 0;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL)
		return -1;
	if (size % 2 != 0)
		return -1;

	string name_buf, value_buf;

	const redis_result* rr;
	for (size_t i = 0; i < size;)
	{
		rr = children[i];
		if (rr->get_type() != REDIS_RESULT_STRING)
		{
			i += 2;
			continue;
		}
		name_buf.clear();
		value_buf.clear();
		rr->argv_to_string(name_buf);
		i++;
		rr->argv_to_string(value_buf);
		i++;
		out[name_buf] = value_buf;
	}

	return (int) out.size();
}

int redis_client::get_strings(std::vector<string>& names,
	std::vector<string>& values)
{
	names.clear();
	values.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;
	if (result->get_size() == 0)
		return 0;

	size_t size;
	const redis_result** children = result->get_children(&size);

	if (children == NULL)
		return -1;
	if (size % 2 != 0)
		return -1;

	string buf;
	const redis_result* rr;

	for (size_t i = 0; i < size;)
	{
		rr = children[i];
		if (rr->get_type() != REDIS_RESULT_STRING)
		{
			i += 2;
			continue;
		}
		buf.clear();
		rr->argv_to_string(buf);
		i++;
		names.push_back(buf);

		buf.clear();
		rr->argv_to_string(buf);
		i++;
		values.push_back(buf);
	}

	return (int) names.size();
}

int redis_client::get_strings(std::vector<const char*>& names,
	std::vector<const char*>& values)
{
	names.clear();
	values.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;
	if (result->get_size() == 0)
		return 0;

	size_t size;
	const redis_result** children = result->get_children(&size);

	if (children == NULL)
		return -1;
	if (size % 2 != 0)
		return -1;

	char* buf;
	size_t len;
	const redis_result* rr;
	std::vector<const redis_result*>::const_iterator cit;

	for (size_t i = 0; i < size;)
	{
		rr = children[i];
		if (rr->get_type() != REDIS_RESULT_STRING)
		{
			i += 2;
			continue;
		}

		len = rr->get_length() + 1;
		buf = (char*) pool_->dbuf_alloc(len);
		rr->argv_to_string(buf, len);
		i++;
		names.push_back(buf);

		len = rr->get_length() + 1;
		buf = (char*) pool_->dbuf_alloc(len);
		rr->argv_to_string(buf, len);
		i++;
		values.push_back(buf);
	}

	return (int) names.size();
}

const char* redis_client::get_value(size_t i, size_t* len /* = NULL */)
{
	if (result_ == NULL || result_->get_type() != REDIS_RESULT_ARRAY)
		return NULL;
	const redis_result* child = result_->get_child(i);
	if (child == NULL)
		return NULL;
	size_t size = child->get_size();
	if (size == 0)
		return NULL;
	if (size == 1)
		return child->get(0, len);

	// 大内存有可能被切片成多个不连续的小内存
	size = child->get_length();
	size++;
	char* buf = (char*) pool_->dbuf_alloc(size);
	size = child->argv_to_string(buf, size);
	if (len)
		*len = size;
	return buf;
}

const redis_result* redis_client::get_child(size_t i) const
{
	if (result_ == NULL || result_->get_type() != REDIS_RESULT_ARRAY)
		return NULL;
	return result_->get_child(i);
}

size_t redis_client::get_size() const
{
	if (result_ == NULL || result_->get_type() != REDIS_RESULT_ARRAY)
		return 0;
	return result_->get_size();
}

//////////////////////////////////////////////////////////////////////////

void redis_client::reset_request()
{
	request_.clear();
	if (req_)
		req_->reset();
}

void redis_client::build_request(size_t argc, const char* argv[], size_t lens[])
{
	if (slice_req_)
		build_request2(argc, argv, lens);
	else
		build_request1(argc, argv, lens);
}

void redis_client::build_request1(size_t argc, const char* argv[], size_t lens[])
{
	request_.format("*%lu\r\n", (unsigned long) argc);
	for (size_t i = 0; i < argc; i++)
	{
		request_.format_append("$%lu\r\n", (unsigned long) lens[i]);
		request_.append(argv[i], lens[i]);
		request_.append("\r\n");
	}
	//printf("%s", request_.c_str());
}

void redis_client::build_request2(size_t argc, const char* argv[], size_t lens[])
{
	size_t size = 1 + argc * 3;
	req_->reserve(size);
	req_->reset();

#define BLEN	32

	char* buf = (char*) pool_->dbuf_alloc(BLEN);
	int  len = safe_snprintf(buf, BLEN, "*%lu\r\n", (unsigned long) argc);
	req_->put(buf, len);

	for (size_t i = 0; i < argc; i++)
	{
		buf = (char*) pool_->dbuf_alloc(BLEN);
		len = safe_snprintf(buf, BLEN, "$%lu\r\n",
			(unsigned long) lens[i]);
		req_->put(buf, len);

		req_->put(argv[i], lens[i]);

		buf = (char*) pool_->dbuf_strdup("\r\n");
		req_->put(buf, 2);
	}
}

/////////////////////////////////////////////////////////////////////////////

void redis_client::build(const char* cmd, const char* key,
	const std::map<string, string>& attrs)
{
	argc_ = 1 + attrs.size() * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	std::map<string, string>::const_iterator cit = attrs.begin();
	for (; cit != attrs.end(); ++cit)
	{
		argv_[i] = cit->first.c_str();
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = cit->second.c_str();
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_client::build(const char* cmd, const char* key,
	const std::map<string, const char*>& attrs)
{
	argc_ = 1 + attrs.size() * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	std::map<string, const char*>::const_iterator cit = attrs.begin();
	for (; cit != attrs.end(); ++cit)
	{
		argv_[i] = cit->first.c_str();
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = cit->second;
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

/////////////////////////////////////////////////////////////////////////////

void redis_client::build(const char* cmd, const char* key,
	const std::map<int, string>& attrs)
{
	argc_ = 1 + attrs.size() * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	std::map<int, string>::const_iterator cit = attrs.begin();
	for (; cit != attrs.end(); ++cit)
	{
		char* tmp = (char*) pool_->dbuf_alloc(INT_LEN);
		(void) safe_snprintf(tmp, INT_LEN, "%d", cit->first);
		argv_[i] = tmp;
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = cit->second.c_str();
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_client::build(const char* cmd, const char* key,
	const std::map<int, const char*>& attrs)
{
	argc_ = 1 + attrs.size() * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	std::map<int, const char*>::const_iterator cit = attrs.begin();
	for (; cit != attrs.end(); ++cit)
	{
		char* tmp = (char*) pool_->dbuf_alloc(INT_LEN);
		(void) safe_snprintf(tmp, INT_LEN, "%d", cit->first);
		argv_[i] = tmp;
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = cit->second;
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

/////////////////////////////////////////////////////////////////////////////

void redis_client::build(const char* cmd, const char* key,
	const std::vector<string>& names, const std::vector<string>& values)
{
	if (names.size() != values.size())
	{
		logger_fatal("names's size: %lu, values's size: %lu",
			(unsigned long) names.size(),
			(unsigned long) values.size());
	}

	argc_ = 1 + names.size() * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	size_t size = names.size();
	for (size_t j = 0; j < size; j++)
	{
		argv_[i] = names[j].c_str();
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = values[j].c_str();
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_client::build(const char* cmd, const char* key,
	const std::vector<const char*>& names,
	const std::vector<const char*>& values)
{
	if (names.size() != values.size())
	{
		logger_fatal("names's size: %lu, values's size: %lu",
			(unsigned long) names.size(),
			(unsigned long) values.size());
	}

	argc_ = 1 + names.size() * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	size_t size = names.size();
	for (size_t j = 0; j < size; j++)
	{
		argv_[i] = names[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = values[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

/////////////////////////////////////////////////////////////////////////////

void redis_client::build(const char* cmd, const char* key,
	const std::vector<int>& names, const std::vector<string>& values)
{
	if (names.size() != values.size())
	{
		logger_fatal("names's size: %lu, values's size: %lu",
			(unsigned long) names.size(),
			(unsigned long) values.size());
	}

	argc_ = 1 + names.size() * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	char* buf4int;
	size_t size = names.size();
	for (size_t j = 0; j < size; j++)
	{
		buf4int = (char*) pool_->dbuf_alloc(INT_LEN);
		(void) safe_snprintf(buf4int, INT_LEN, "%d", names[j]);
		argv_[i] = buf4int;
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = values[j].c_str();
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_client::build(const char* cmd, const char* key,
	const std::vector<int>& names, const std::vector<const char*>& values)
{
	if (names.size() != values.size())
	{
		logger_fatal("names's size: %lu, values's size: %lu",
			(unsigned long) names.size(),
			(unsigned long) values.size());
	}

	argc_ = 1 + names.size() * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	char* buf4int;
	size_t size = names.size();
	for (size_t j = 0; j < size; j++)
	{
		buf4int = (char*) pool_->dbuf_alloc(INT_LEN);
		(void) safe_snprintf(buf4int, INT_LEN, "%d", names[j]);
		argv_[i] = buf4int;
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = values[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

/////////////////////////////////////////////////////////////////////////////

void redis_client::build(const char* cmd, const char* key,
	const char* names[], const char* values[], size_t argc)
{
	argc_ = 1 + argc * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	for (size_t j = 0; j < argc; j++)
	{
		argv_[i] = names[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = values[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_client::build(const char* cmd, const char* key,
	const int names[], const char* values[], size_t argc)
{
	argc_ = 1 + argc * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	char* buf4int;
	for (size_t j = 0; j < argc; j++)
	{
		buf4int = (char*) pool_->dbuf_alloc(INT_LEN);
		(void) safe_snprintf(buf4int, INT_LEN, "%d", names[j]);
		argv_[i] = buf4int;
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = values[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_client::build(const char* cmd, const char* key,
	const char* names[], const size_t names_len[],
	const char* values[], const size_t values_len[], size_t argc)
{
	argc_ = 1 + argc * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	for (size_t j = 0; j < argc; j++)
	{
		argv_[i] = names[j];
		argv_lens_[i] = names_len[j];
		i++;

		argv_[i] = values[j];
		argv_lens_[i] = values_len[j];
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

/////////////////////////////////////////////////////////////////////////////

void redis_client::build(const char* cmd, const char* key,
	const std::vector<string>& names)
{
	size_t argc = names.size();
	argc_ = 1 + argc;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	for (size_t j = 0; j < argc; j++)
	{
		argv_[i] = names[j].c_str();
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_client::build(const char* cmd, const char* key,
	const std::vector<const char*>& names)
{
	size_t argc = names.size();
	argc_ = 1 + argc;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	for (size_t j = 0; j < argc; j++)
	{
		argv_[i] = names[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_client::build(const char* cmd, const char* key,
	const std::vector<int>& names)
{
	size_t argc = names.size();
	argc_ = 1 + argc;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	char* buf4int;
	for (size_t j = 0; j < argc; j++)
	{
		buf4int = (char*) pool_->dbuf_alloc(INT_LEN);
		safe_snprintf(buf4int, INT_LEN, "%d", names[j]);
		argv_[i] = buf4int;
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_client::build(const char* cmd, const char* key,
	const char* names[], size_t argc)
{
	argc_ = 1 + argc;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	for (size_t j = 0; j < argc; j++)
	{
		argv_[i] = names[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_client::build(const char* cmd, const char* key,
	const int names[], size_t argc)
{
	argc_ = 1 + argc;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	char* buf4int;
	for (size_t j = 0; j < argc; j++)
	{
		buf4int = (char*) pool_->dbuf_alloc(INT_LEN);
		safe_snprintf(buf4int, INT_LEN, "%d", names[j]);
		argv_[i] = buf4int;
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_client::build(const char* cmd, const char* key,
	const char* names[], const size_t lens[], size_t argc)
{
	argc_ = 1 + argc;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	for (size_t j = 0; j < argc; j++)
	{
		argv_[i] = names[j];
		argv_lens_[i] = lens[j];
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

/////////////////////////////////////////////////////////////////////////////

} // end namespace acl
