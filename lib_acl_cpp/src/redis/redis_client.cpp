#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_client.hpp"

namespace acl
{

#define INT_LEN	11

redis_client::redis_client(const char* addr, int conn_timeout /* = 60 */,
	int rw_timeout /* = 30 */, bool retry /* = true */)
: used_(0)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
, retry_(retry)
, argv_size_(0)
, argv_(NULL)
, argv_lens_(NULL)
, result_(NULL)
{
	addr_ = acl_mystrdup(addr);
	pool_ = NEW dbuf_pool();
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

redis_result* redis_client::get_error()
{
	buf_.clear();
	if (conn_.gets(buf_) == false)
		return NULL;

	redis_result* rr = new(pool_) redis_result(pool_);
	rr->set_type(REDIS_RESULT_ERROR);
	rr->set_size(1);
	rr->put(buf_.c_str(), buf_.length());
	return rr;
}

redis_result* redis_client::get_status()
{
	buf_.clear();
	if (conn_.gets(buf_) == false)
		return NULL;

	redis_result* rr = new(pool_) redis_result(pool_);
	rr->set_type(REDIS_RESULT_STATUS);
	rr->set_size(1);
	rr->put(buf_.c_str(), buf_.length());
	return rr;
}

redis_result* redis_client::get_integer()
{
	buf_.clear();
	if (conn_.gets(buf_) == false)
		return NULL;

	redis_result* rr = new(pool_) redis_result(pool_);
	rr->set_type(REDIS_RESULT_INTEGER);
	rr->set_size(1);
	rr->put(buf_.c_str(), buf_.length());
	return rr;
}

redis_result* redis_client::get_string()
{
	buf_.clear();
	if (conn_.gets(buf_) == false)
		return NULL;
	redis_result* rr = new(pool_) redis_result(pool_);
	rr->set_type(REDIS_RESULT_STRING);
	int len = atoi(buf_.c_str());
	if (len < 0)
		return rr;

	// 将可能返回的大内存分成不连接的内存链存储

#define CHUNK_LENGTH	8192

	size_t size = (size_t) len / CHUNK_LENGTH;
	if ((size_t) len % CHUNK_LENGTH != 0)
		size++;
	rr->set_size(size);
	char*  buf;
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

redis_result* redis_client::get_array()
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
		redis_result* child = get_object();
		if (child == NULL)
			return NULL;
		rr->put(child, i);
	}

	return rr;
}

redis_result* redis_client::get_object()
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
		return get_error();
	case '+':	// STATUS
		return get_status();
	case ':':	// INTEGER
		return get_integer();
	case '$':	// STRING
		return get_string();
	case '*':	// ARRAY
		return get_array();
	default:	// INVALID
		logger_error("invalid first char: %c, %d", ch, ch);
		return NULL;
	}
}

redis_result* redis_client::get_objects(size_t nobjs)
{
	acl_assert(nobjs > 1);

	redis_result* objs = new(pool_) redis_result(pool_);
	objs->set_type(REDIS_RESULT_ARRAY);
	objs->set_size(nobjs);

	for (size_t i = 0; i < nobjs; i++)
	{
		redis_result* obj = get_object();
		if (obj == NULL)
			return NULL;
		objs->put(obj, i);
	}
	return objs;
}

const redis_result* redis_client::run(const string& request,
	size_t nobjs /* = 1 */)
{
	// 本连接使用次数递增
	used_++;

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

		if (!request.empty() && conn_.write(request) == -1)
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

		if (nobjs > 1)
			result_ = get_objects(nobjs);
		else
			result_ = get_object();

		if (result_ != NULL)
			return result_;
		conn_.close();

		if (!retry_ || retried)
			break;
	}

	return NULL;
}

const string& redis_client::build_request(size_t argc, const char* argv[],
	size_t argv_lens[], string* buf /* = NULL */)
{
	if (buf == NULL)
	{
		buf = &request_;
		buf->clear();
	}

	buf->format_append("*%lu\r\n", (unsigned long) argc);
	for (size_t i = 0; i < argc; i++)
	{
		buf->format_append("$%lu\r\n", (unsigned long) argv_lens[i]);
		buf->append(argv[i], argv_lens[i]);
		buf->append("\r\n");
	}
	return *buf;
}

const string& redis_client::build_request(const std::vector<string>& args,
	string* buf /* = NULL */)
{
	if (buf == NULL)
	{
		buf = &request_;
		buf->clear();
	}

	buf->format("*%lu\r\n", (unsigned long) args.size());
	std::vector<string>::const_iterator cit = args.begin();
	for (; cit != args.end(); ++cit)
	{
		buf->format_append("$%lu\r\n", (unsigned long) (*cit).size());
		buf->append(*cit);
		buf->append("\r\n");
	}
	return *buf;
}

const string& redis_client::build_request(const std::vector<const char*>& args,
	const std::vector<size_t>& lens, string* buf /* = NULL */)
{
	acl_assert(args.size() == lens.size());

	if (buf == NULL)
	{
		buf = &request_;
		buf->clear();
	}

	buf->format("*%lu\r\n", (unsigned long) args.size());
	std::vector<const char*>::const_iterator args_cit = args.begin();
	std::vector<size_t>::const_iterator lens_cit = lens.begin();
	for (; args_cit != args.end(); ++args_cit, ++lens_cit)
	{
		buf->format_append("$%lu\r\n", (unsigned long) *lens_cit);
		buf->append(*args_cit, *lens_cit);
		buf->append("\r\n");
	}
	return *buf;
}

/////////////////////////////////////////////////////////////////////////////

/***************************************************************************/
/***************************************************************************/

const string& redis_client::build(const char* cmd, const char* key,
	const std::map<string, string>& attrs, string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

const string& redis_client::build(const char* cmd, const char* key,
	const std::map<string, char*>& attrs, string* buf /* = NULL */)
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

	std::map<string, char*>::const_iterator cit = attrs.begin();
	for (; cit != attrs.end(); ++cit)
	{
		argv_[i] = cit->first.c_str();
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = cit->second;
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	return build_request(argc_, argv_, argv_lens_, buf);
}

const string& redis_client::build(const char* cmd, const char* key,
	const std::map<string, const char*>& attrs, string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

/////////////////////////////////////////////////////////////////////////////

const string& redis_client::build(const char* cmd, const char* key,
	const std::map<int, string>& attrs, string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

const string& redis_client::build(const char* cmd, const char* key,
	const std::map<int, char*>& attrs, string* buf /* = NULL */)
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

	std::map<int, char*>::const_iterator cit = attrs.begin();
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

const string& redis_client::build(const char* cmd, const char* key,
	const std::map<int, const char*>& attrs, string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

/////////////////////////////////////////////////////////////////////////////

const string& redis_client::build(const char* cmd, const char* key,
	const std::vector<string>& names, const std::vector<string>& values,
	string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

const string& redis_client::build(const char* cmd, const char* key,
	const std::vector<char*>& names, const std::vector<char*>& values,
	string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

const string& redis_client::build(const char* cmd, const char* key,
	const std::vector<const char*>& names,
	const std::vector<const char*>& values, string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

/////////////////////////////////////////////////////////////////////////////

const string& redis_client::build(const char* cmd, const char* key,
	const std::vector<int>& names, const std::vector<string>& values,
	string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

const string& redis_client::build(const char* cmd, const char* key,
	const std::vector<int>& names, const std::vector<char*>& values,
	string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

const string& redis_client::build(const char* cmd, const char* key,
	const std::vector<int>& names, const std::vector<const char*>& values,
	string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

/////////////////////////////////////////////////////////////////////////////

const string& redis_client::build(const char* cmd, const char* key,
	const char* names[], const char* values[], size_t argc,
	string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

const string& redis_client::build(const char* cmd, const char* key,
	const int names[], const char* values[], size_t argc,
	string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

const string& redis_client::build(const char* cmd, const char* key,
	const char* names[], const size_t names_len[],
	const char* values[], const size_t values_len[],
	size_t argc, string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

/////////////////////////////////////////////////////////////////////////////

/***************************************************************************/
/*                        for other request                                */
/***************************************************************************/

const string& redis_client::build(const char* cmd, const char* key,
	const std::vector<string>& names, string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

const string& redis_client::build(const char* cmd, const char* key,
	const std::vector<char*>& names, string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

const string& redis_client::build(const char* cmd, const char* key,
	const std::vector<const char*>& names, string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

const string& redis_client::build(const char* cmd, const char* key,
	const std::vector<int>& names, string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

const string& redis_client::build(const char* cmd, const char* key,
	const char* names[], size_t argc, string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

const string& redis_client::build(const char* cmd, const char* key,
	const int names[], size_t argc, string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

const string& redis_client::build(const char* cmd, const char* key,
	const char* names[], const size_t lens[],
	size_t argc, string* buf /* = NULL */)
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

	return build_request(argc_, argv_, argv_lens_, buf);
}

/////////////////////////////////////////////////////////////////////////////

} // end namespace acl
