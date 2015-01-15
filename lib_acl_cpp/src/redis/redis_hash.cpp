#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_hash.hpp"

namespace acl
{

#define INT64_LEN	21
#define FLOAT_LEN	32

redis_hash::redis_hash(redis_client& conn)
: conn_(conn)
, result_(NULL)
{

}

redis_hash::~redis_hash()
{
}

/////////////////////////////////////////////////////////////////////////////

bool redis_hash::hmset(const char* key, const std::map<string, string>& attrs)
{
	const string& req = conn_.build("HMSET", key, attrs);
	return hmset(req);
}

bool redis_hash::hmset(const char* key, const std::map<string, char*>& attrs)
{
	const string& req = conn_.build("HMSET", key, attrs);
	return hmset(req);
}

bool redis_hash::hmset(const char* key, const std::map<string, const char*>& attrs)
{
	const string& req = conn_.build("HMSET", key, attrs);
	return hmset(req);
}

bool redis_hash::hmset(const char* key, const std::map<int, string>& attrs)
{
	const string& req = conn_.build("HMSET", key, attrs);
	return hmset(req);
}

bool redis_hash::hmset(const char* key, const std::map<int, char*>& attrs)
{
	const string& req = conn_.build("HMSET", key, attrs);
	return hmset(req);
}

bool redis_hash::hmset(const char* key, const std::map<int, const char*>& attrs)
{
	const string& req = conn_.build("HMSET", key, attrs);
	return hmset(req);
}

bool redis_hash::hmset(const string& req)
{
	result_ = conn_.run(req);
	if (result_ == NULL)
		return false;
	const char* res = result_->get(0);
	if (res == NULL || strcasecmp(res, "ok") != 0)
		return false;
	return true;
}

/////////////////////////////////////////////////////////////////////////////

bool redis_hash::hmget(const char* key, const std::vector<string>& names,
	std::vector<string>* result /* = NULL */)
{
	const string& req = conn_.build("HMGET", key, names);
	return hmget(req, result);
}

bool redis_hash::hmget(const char* key, const std::vector<char*>& names,
	std::vector<string>* result /* = NULL */)
{
	const string& req = conn_.build("HMGET", key, names);
	return hmget(req, result);
}

bool redis_hash::hmget(const char* key, const std::vector<const char*>& names,
	std::vector<string>* result /* = NULL */)
{
	const string& req = conn_.build("HMGET", key, names);
	return hmget(req, result);
}

bool redis_hash::hmget(const char* key, const std::vector<int>& names,
	std::vector<string>* result /* = NULL */)
{
	const string& req = conn_.build("HMGET", key, names);
	return hmget(req, result);
}

bool redis_hash::hmget(const char* key, const char* names[], size_t argc,
	std::vector<string>* result /* = NULL */)
{
	const string& req = conn_.build("HMGET", key, names, argc);
	return hmget(req, result);
}

bool redis_hash::hmget(const char* key, const int names[], size_t argc,
	std::vector<string>* result /* = NULL */)
{
	const string& req = conn_.build("HMGET", key, names, argc);
	return hmget(req, result);
}

bool redis_hash::hmget(const char* key, const char* names[],
	const size_t lens[], size_t argc, std::vector<string>* result /* = NULL */)
{
	const string& req = conn_.build("HMGET", key, names, lens, argc);
	return hmget(req, result);
}

bool redis_hash::hmget(const string& req, std::vector<string>* result /* = NULL */)
{
	result_ = conn_.run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_ARRAY)
		return false;
	if (result == NULL)
		return true;

	size_t size = hmget_size();
	const redis_result* rr;
	size_t nslice, len;
	const char* ptr;
	string buf(4096);

	for (size_t i = 0; i < size; i++)
	{
		rr = hmget_result(i);
		if (rr == NULL || (nslice = rr->get_size()) == 0)
			result->push_back("");
		else if (nslice == 1)
		{
			ptr = rr->get(0, &len);
			buf.copy(ptr, len);
			result->push_back(buf);
		}
		else
		{
			buf.clear();
			rr->argv_to_string(buf);
			result->push_back(buf);
		}
	}

	return true;
}

const redis_result* redis_hash::hmget_result(size_t i) const
{
	if (result_ == NULL)
		return NULL;
	if (result_->get_type() != REDIS_RESULT_ARRAY)
		return NULL;
	return result_->get_child(i);
}

const char* redis_hash::hmget_value(size_t i, size_t* len /* = NULL */) const
{
	if (result_ == NULL)
		return NULL;
	if (result_->get_type() != REDIS_RESULT_ARRAY)
		return NULL;
	const redis_result* rr = result_->get_child(i);
	if (rr == NULL)
		return NULL;
	size_t size = rr->get_size();
	if (size == 0)
		return NULL;
	if (size == 1)
		return rr->get(0, len);

	// 大内存有可能被切片成多个不连续的小内存
	size = rr->get_length();
	size++;
	char* buf = (char*) conn_.get_pool()->dbuf_alloc(size);
	size = rr->argv_to_string(buf, size);
	if (len)
		*len = size;
	return buf;
}

size_t redis_hash::hmget_size() const
{
	if (result_ == NULL)
		return 0;
	if (result_->get_type() != REDIS_RESULT_ARRAY)
		return 0;
	return result_->get_size();
}

/////////////////////////////////////////////////////////////////////////////

int redis_hash::hset(const char* key, const char* name, const char* value)
{
	return hset(key, name, value, strlen(value));
}

int redis_hash::hset(const char* key, const char* name,
	const char* value, size_t value_len)
{
	return hset(key, name, strlen(name), value, value_len);
}

int redis_hash::hset(const char* key, const char* name, size_t name_len,
	const char* value, size_t value_len)
{
	const char* names[1];
	size_t names_len[1];

	names[0] = name;
	names_len[0] = name_len;

	const char* values[1];
	size_t values_len[1];

	values[0] = value;
	values_len[0] = value_len;

	const string& req = conn_.build("HSET", key, names, names_len,
		values, values_len, 1);
	return hset(req);
}

int redis_hash::hset(const string& req)
{
	result_ = conn_.run(req);
	if (result_ == NULL)
		return -1;
	if (result_->get_type() != REDIS_RESULT_INTEGER)
		return -1;
	bool success;
	int ret = result_->get_integer(&success);
	if (!success)
		return -1;
	if (ret != 0 && ret != 1)
		return -1;
	return ret;
}

int redis_hash::hsetnx(const char* key, const char* name, const char* value)
{
	return hsetnx(key, name, value, strlen(value));
}

int redis_hash::hsetnx(const char* key, const char* name,
		      const char* value, size_t value_len)
{
	return hsetnx(key, name, strlen(name), value, value_len);
}

int redis_hash::hsetnx(const char* key, const char* name, size_t name_len,
	const char* value, size_t value_len)
{
	const char* names[1];
	names[0] = name;
	size_t names_len[1];
	names_len[0] = name_len;

	const char* values[1];
	values[0] = value;
	size_t values_len[1];
	values_len[0] = value_len;

	const string& req = conn_.build("HSETNX", key, names, names_len,
		values, values_len, 1);
	return hsetnx(req);
}

int redis_hash::hsetnx(const string& req)
{
	result_ = conn_.run(req);
	if (result_ == NULL)
		return -1;
	if (result_->get_type() != REDIS_RESULT_INTEGER)
		return -1;
	bool success;
	int ret = result_->get_integer(&success);
	if (!success)
		return -1;
	return ret;
}

bool redis_hash::hget(const char* key, const char* name, string& result)
{
	return hget(key, name, strlen(name), result);
}

bool redis_hash::hget(const char* key, const char* name,
	size_t name_len, string& result)
{
	const char* names[1];
	names[0] = name;
	size_t names_len[1];
	names_len[0] = name_len;

	const string& req = conn_.build("HGET", key, names, names_len, 1);
	const redis_result* rr = conn_.run(req);
	if (rr == NULL)
		return false;
	if (rr->get_type() != REDIS_RESULT_STRING)
		return false;
	rr->argv_to_string(result);
	return true;
}

bool redis_hash::hgetall(const char* key, std::map<string, string>& result)
{
	const char* keys[1];
	keys[0] = key;
	const string& req = conn_.build("HGETALL", NULL, keys, 1);
	const redis_result* rr = conn_.run(req);
	if (rr == NULL)
		return false;
	if (rr->get_type() != REDIS_RESULT_ARRAY)
		return false;
	if (rr->get_size() == 0)
		return true;

	size_t size;
	const redis_result** children = rr->get_children(&size);
	if (children == NULL)
		return false;
	if (size % 2 != 0)
		return false;

	string name_buf, value_buf;

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
		result[name_buf] = value_buf;
	}
	return true;
}

bool redis_hash::hgetall(const char* key, std::vector<string>& names,
	std::vector<string>& values)
{
	const char* keys[1];
	keys[0] = key;
	const string& req = conn_.build("HGETALL", NULL, keys, 1);
	const redis_result* rr = conn_.run(req);
	if (rr == NULL)
		return false;
	if (rr->get_type() != REDIS_RESULT_ARRAY)
		return false;
	if (rr->get_size() == 0)
		return true;

	size_t size;
	const redis_result** children = rr->get_children(&size);

	if (children == NULL)
		return false;
	if (size % 2 != 0)
		return false;

	string buf;

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
	return true;
}

bool redis_hash::hgetall(const char* key, std::vector<const char*>& names,
	std::vector<const char*>& values)
{
	const char* keys[1];
	keys[0] = key;
	const string& req = conn_.build("HGETALL", NULL, keys, 1);
	const redis_result* rr = conn_.run(req);
	if (rr == NULL)
		return false;
	if (rr->get_type() != REDIS_RESULT_ARRAY)
		return false;
	if (rr->get_size() == 0)
		return true;

	size_t size;
	const redis_result** children = rr->get_children(&size);

	if (children == NULL)
		return false;
	if (size % 2 != 0)
		return false;

	char* buf;
	size_t len;
	dbuf_pool* pool = conn_.get_pool();
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
		buf = (char*) pool->dbuf_alloc(len);
		rr->argv_to_string(buf, len);
		i++;
		names.push_back(buf);

		len = rr->get_length() + 1;
		buf = (char*) pool->dbuf_alloc(len);
		rr->argv_to_string(buf, len);
		i++;
		values.push_back(buf);
	}
	return true;
}

int redis_hash::hdel(const char* key, const char* first_name, ...)
{
	const char* name;
	std::vector<const char*> names;
	names.push_back(first_name);
	va_list ap;
	va_start(ap, first_name);
	while((name = va_arg(ap, const char*)) != NULL)
		names.push_back(name);
	return hdel(key, names);
}

int redis_hash::hdel(const char* key, const char* names[], size_t argc)
{
	const string& req = conn_.build("HDEL", key, names, argc);
	return hdel(req);
}

int redis_hash::hdel(const char* key, const char* names[],
	const size_t names_len[], size_t argc)
{
	const string& req = conn_.build("HDEL", key, names, names_len, argc);
	return hdel(req);;
}

int redis_hash::hdel(const char* key, const std::vector<string>& names)
{
	const string& req = conn_.build("HDEL", key, names);
	return hdel(req);
}

int redis_hash::hdel(const char* key, const std::vector<char*>& names)
{
	const string& req = conn_.build("HDEL", key, names);
	return hdel(req);
}

int redis_hash::hdel(const char* key, const std::vector<const char*>& names)
{
	const string& req = conn_.build("HDEL", key, names);
	return hdel(req);
}

int redis_hash::hdel(const string& req)
{
	result_ = conn_.run(req);
	if (result_ == NULL)
		return -1;
	if (result_->get_type() != REDIS_RESULT_INTEGER)
		return -1;
	return result_->get_integer();
}

bool redis_hash::hincrby(const char* key, const char* name,
	long long int inc, long long int* result /* = NULL */)
{
	const char* names[1];
	names[0] = name;
	char buf[INT64_LEN];
	(void) acl_i64toa(inc, buf, sizeof(buf));
	const char* values[1];
	values[0] = buf;
	const string& req = conn_.build("HINCRBY", key, names, values, 1);
	result_ = conn_.run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_INTEGER)
		return false;
	if (result != NULL)
		*result = result_->get_integer64();
	return true;
}

bool redis_hash::hincrbyfloat(const char* key, const char* name,
	double inc, double* result /* = NULL */)
{
	const char* names[1];
	names[0] = name;
	char buf[FLOAT_LEN];
	(void) safe_snprintf(buf, sizeof(buf), "%f", inc);
	const char* values[1];
	values[0] = buf;
	const string& req = conn_.build("HINCRBYFLOAT", key, names, values, 1);
	result_ = conn_.run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_STRING)
		return false;
	if (result != NULL)
	{
		(void) result_->argv_to_string(buf, sizeof(buf));
		*result = atof(buf);
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////

bool redis_hash::hkeys(const char* key, std::vector<string>& names)
{
	const char* keys[1];
	keys[0] = key;
	const string& req = conn_.build("HKEYS", NULL, keys, 1);
	result_ = conn_.run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_ARRAY)
		return false;

	size_t size;
	const redis_result** children = result_->get_children(&size);

	if (children == NULL)
		return false;

	string buf;
	const redis_result* rr;

	for (size_t i = 0; i < size; i++)
	{
		rr = children[i];
		if (rr->get_type() != REDIS_RESULT_STRING)
			continue;
		buf.clear();
		rr->argv_to_string(buf);
		names.push_back(buf);
	}

	return true;
}

bool redis_hash::hexists(const char* key, const char* name)
{
	return hexists(key, name, strlen(name));
}

bool redis_hash::hexists(const char* key, const char* name, size_t name_len)
{
	const char* names[1];
	names[0] = name;
	size_t names_len[1];
	names_len[0] = name_len;

	const string& req = conn_.build("HEXISTS", key, names, names_len, 1);
	const redis_result* rr = conn_.run(req);
	if (rr == NULL)
		return false;
	if (rr->get_type() != REDIS_RESULT_INTEGER)
		return false;
	int ret = rr->get_integer();
	if (ret == 1)
		return true;
	else
		return false;
}

int redis_hash::hlen(const char* key)
{
	const char* keys[1];
	keys[0] = key;
	const string& req = conn_.build("HLEN", NULL, keys, 1);
	const redis_result* rr = conn_.run(req);
	if (rr == NULL)
		return -1;
	if (rr->get_type() != REDIS_RESULT_INTEGER)
		return -1;
	return rr->get_integer();
}

/////////////////////////////////////////////////////////////////////////////

} // namespace acl
