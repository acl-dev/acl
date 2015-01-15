#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_string.hpp"

namespace acl
{

#define INT64_LEN	21
#define INT_LEN		11
#define FLOAT_LEN	32

redis_string::redis_string(redis_client& conn)
: conn_(conn)
, result_(NULL)
{

}

redis_string::~redis_string()
{

}

bool redis_string::set(const char* key, const char* value)
{
	return set(key, strlen(key), value, strlen(value));
}

bool redis_string::set(const char* key, size_t key_len,
	const char* value, size_t value_len)
{
	const char* names[2];
	size_t lens[2];

	names[0] = key;
	lens[0] = key_len;

	names[1] = value;
	lens[1] = value_len;

	const string& req = conn_.build("SET", NULL, names, lens, 2);
	result_ = conn_.run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_STATUS)
		return false;
	const char* status = result_->get_status();
	if (status == NULL || strcasecmp(status, "OK") != 0)
		return false;
	else
		return true;
}

bool redis_string::setex(const char* key, const char* value, int timeout)
{
	return setex(key, strlen(key), value, strlen(value), timeout);
}

bool redis_string::setex(const char* key, size_t key_len, const char* value,
	size_t value_len, int timeout)
{
	const char* names[3];
	size_t lens[3];

	names[0] = key;
	lens[0] = key_len;

	char buf[INT_LEN];
	(void) safe_snprintf(buf, sizeof(buf), "%d", timeout);
	names[1] = buf;
	lens[1] = strlen(buf);

	names[2] = value;
	lens[2] = value_len;

	const string& req = conn_.build("SETEX", NULL, names, lens, 3);
	result_ = conn_.run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_STATUS)
		return false;
	const char* status = result_->get_status();
	if (status == NULL || strcasecmp(status, "OK") != 0)
		return false;
	else
		return true;
}

int redis_string::setnx(const char* key, const char* value)
{
	return setnx(key, strlen(key), value, strlen(value));
}

int redis_string::setnx(const char* key, size_t key_len,
	const char* value, size_t value_len)
{
	const char* names[2];
	size_t lens[2];

	names[0] = key;
	lens[0] = key_len;

	names[1] = value;
	lens[1] = value_len;

	const string& req = conn_.build("SETNX", NULL, names, lens, 2);
	result_ = conn_.run(req);
	if (result_ == NULL)
		return -1;
	if (result_->get_type() != REDIS_RESULT_INTEGER)
		return -1;
	return result_->get_integer();
}

int redis_string::append(const char* key, const char* value)
{
	return append(key, value, strlen(value));
}

int redis_string::append(const char* key, const char* value, size_t size)
{
	const char* names[2];
	size_t lens[2];

	names[0] = key;
	lens[0] = strlen(key);

	names[1] = value;
	lens[1] = size;

	const string& req = conn_.build("APPEND", NULL, names, lens, 2);
	result_ = conn_.run(req);
	if (result_ == NULL)
		return -1;
	if (result_->get_type() != REDIS_RESULT_INTEGER)
		return -1;
	return result_->get_integer();
}

bool redis_string::get(const char* key, string& buf)
{
	result_ = get(key);
	if (result_ == NULL)
		return false;
	(void) result_->argv_to_string(buf);
	return true;
}

const redis_result* redis_string::get(const char* key)
{
	const char* keys[1];
	keys[0] = key;

	const string& req = conn_.build("GET", NULL, keys, 1);
	result_ = conn_.run(req);
	if (result_ == NULL)
		return NULL;
	if (result_->get_type() != REDIS_RESULT_STRING)
		return NULL;
	return result_;
}

bool redis_string::getset(const char* key, const char* value, string& buf)
{
	return getset(key, strlen(key), value, strlen(value), buf);
}

bool redis_string::getset(const char* key, size_t key_len,
	const char* value, size_t value_len, string& buf)
{
	const char* names[2];
	size_t lens[2];

	names[0] = key;
	lens[0] = key_len;

	names[1] = value;
	lens[1] = value_len;

	const string& req = conn_.build("GETSET", NULL, names, lens, 2);
	result_ = conn_.run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_STRING)
		return false;
	(void) result_->argv_to_string(buf);
	return true;
}

/////////////////////////////////////////////////////////////////////////////

int redis_string::str_len(const char* key)
{
	return str_len(key, strlen(key));
}

int redis_string::str_len(const char* key, size_t len)
{
	const char* names[1];
	size_t lens[1];

	names[0] = key;
	lens[0] = len;

	const string& req = conn_.build("STRLEN", NULL, names, lens, 1);
	result_ = conn_.run(req);
	if (result_ == NULL)
		return -1;
	if (result_->get_type() != REDIS_RESULT_INTEGER)
		return -1;
	return result_->get_integer();
}

int redis_string::setrange(const char* key, unsigned offset, const char* value)
{
	return setrange(key, strlen(key), offset, value, strlen(value));
}

int redis_string::setrange(const char* key, size_t key_len, unsigned offset,
	const char* value, size_t value_len)
{
	const char* names[3];
	size_t lens[3];

	names[0] = key;
	lens[0] = key_len;

	char buf[INT64_LEN];
	(void) acl_i64toa(offset, buf, sizeof(buf));
	names[1] = buf;
	lens[1] = strlen(buf);

	names[2] = value;
	lens[2] = value_len;

	const string& req = conn_.build("SETRANGE", NULL, names, lens, 3);
	result_ = conn_.run(req);
	if (result_ == NULL)
		return -1;

	if (result_->get_type() != REDIS_RESULT_INTEGER)
		return -1;
	return result_->get_integer();
}

bool redis_string::getrange(const char* key, int start, int end, string& buf)
{
	return getrange(key, strlen(key), start, end, buf);
}

bool redis_string::getrange(const char* key, size_t key_len,
	int start, int end, string& buf)
{
	const char* names[3];
	size_t lens[3];

	names[0] = key;
	lens[0] = key_len;

	char start_buf[INT_LEN], end_buf[INT_LEN];
	(void) safe_snprintf(start_buf, sizeof(start_buf), "%d", start);
	names[1] = start_buf;
	lens[1] = strlen(start_buf);

	(void) safe_snprintf(end_buf, sizeof(end_buf), "%d", end);
	names[2] = end_buf;
	lens[2] = strlen(end_buf);

	const string& req = conn_.build("GETRANGE", NULL, names, lens, 3);
	result_ = conn_.run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_STRING)
		return false;
	(void) result_->argv_to_string(buf);
	return true;
}

/////////////////////////////////////////////////////////////////////////////

bool redis_string::setbit(const char* key, unsigned offset, int bit)
{
	return setbit(key, strlen(key), offset, bit);
}

bool redis_string::setbit(const char* key, size_t len, unsigned offset, int bit)
{
	const char* names[3];
	size_t lens[3];

	names[0] = key;
	lens[0] = len;

	char buf4off[INT_LEN];
	(void) safe_snprintf(buf4off, sizeof(buf4off), "%d", offset);
	names[1] = buf4off;
	lens[1] = strlen(buf4off);

	names[2] = bit ? "1" : "0";
	lens[2] = 1;

	const string& req = conn_.build("SETBIT", NULL, names, lens, 3);
	result_ = conn_.run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_INTEGER)
		return false;
	return true;
}

bool redis_string::getbit(const char* key, unsigned offset, int& bit)
{
	return getbit(key, strlen(key), offset, bit);
}

bool redis_string::getbit(const char* key, size_t len, unsigned offset, int& bit)
{
	const char* names[2];
	size_t lens[2];

	names[0] = key;
	lens[0] = len;

	char buf4off[INT_LEN];
	(void) safe_snprintf(buf4off, sizeof(buf4off), "%d", offset);
	names[1] = buf4off;
	lens[1] = strlen(buf4off);

	const string& req = conn_.build("GETBIT", NULL, names, lens, 2);
	result_ = conn_.run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_INTEGER)
		return false;

	bit = result_->get_integer() == 0 ? 0 : 1;
	return true;
}

int redis_string::bitcount(const char* key)
{
	return bitcount(key, strlen(key));
}

int redis_string::bitcount(const char* key, size_t len)
{
	const char* names[1];
	size_t lens[1];

	names[0] = key;
	lens[0] = len;

	const string& req = conn_.build("BITCOUNT", NULL, names, lens, 1);
	result_ = conn_.run(req);
	if (result_ == NULL)
		return -1;
	if (result_->get_type() != REDIS_RESULT_INTEGER)
		return -1;
	return result_->get_integer();
}

int redis_string::bitcount(const char* key, int start, int end)
{
	return bitcount(key, strlen(key), start, end);
}

int redis_string::bitcount(const char* key, size_t len, int start, int end)
{
	const char* names[3];
	size_t lens[3];

	names[0] = key;
	lens[0] = len;

	char buf4start[INT_LEN];
	(void) safe_snprintf(buf4start, sizeof(buf4start), "%d", start);
	names[1] = buf4start;
	lens[1] = strlen(buf4start);

	char buf4end[INT_LEN];
	(void) safe_snprintf(buf4end, sizeof(buf4end), "%d", end);
	names[2] = buf4end;
	lens[2] = strlen(buf4end);

	const string& req = conn_.build("BITCOUNT", NULL, names, lens, 3);
	result_ = conn_.run(req);
	if (result_ == NULL)
		return -1;
	if (result_->get_type() != REDIS_RESULT_INTEGER)
		return -1;
	return result_->get_integer();
}

int redis_string::bitop_and(const char* destkey, const std::vector<string>& keys)
{
	return bitop("AND", destkey, keys);
}
	
int redis_string::bitop_or(const char* destkey, const std::vector<string>& keys)
{
	return bitop("OR", destkey, keys);
}

int redis_string::bitop_xor(const char* destkey, const std::vector<string>& keys)
{
	return bitop("XOR", destkey, keys);
}

int redis_string::bitop_and(const char* destkey, const std::vector<const char*>& keys)
{
	return bitop("AND", destkey, keys);
}

int redis_string::bitop_or(const char* destkey, const std::vector<const char*>& keys)
{
	return bitop("OR", destkey, keys);
}

int redis_string::bitop_xor(const char* destkey, const std::vector<const char*>& keys)
{
	return bitop("XOR", destkey, keys);
}

int redis_string::bitop_and(const char* destkey, const char* keys[], size_t size)
{
	return bitop("AND", destkey, keys, size);
}

int redis_string::bitop_or(const char* destkey, const char* keys[], size_t size)
{
	return bitop("OR", destkey, keys, size);
}

int redis_string::bitop_xor(const char* destkey, const char* keys[], size_t size)
{
	return bitop("XOR", destkey, keys, size);
}

int redis_string::bitop_and(const char* destkey, const char* key, ...)
{
	std::vector<const char*> keys;
	va_list ap;
	va_start(ap, key);
	const char* ptr;
	while ((ptr = va_arg(ap, const char*)) != NULL)
		keys.push_back(ptr);
	va_end(ap);
	return bitop("AND", destkey, keys);
}

int redis_string::bitop_or(const char* destkey, const char* key, ...)
{
	std::vector<const char*> keys;
	va_list ap;
	va_start(ap, key);
	const char* ptr;
	while ((ptr = va_arg(ap, const char*)) != NULL)
		keys.push_back(ptr);
	va_end(ap);
	return bitop("OR", destkey, keys);
}

int redis_string::bitop_xor(const char* destkey, const char* key, ...)
{
	std::vector<const char*> keys;
	va_list ap;
	va_start(ap, key);
	const char* ptr;
	while ((ptr = va_arg(ap, const char*)) != NULL)
		keys.push_back(ptr);
	va_end(ap);
	return bitop("XOR", destkey, keys);
}

int redis_string::bitop(const char* op, const char* destkey,
	const std::vector<string>& keys)
{
	std::vector<string> names;
	names.push_back(op);
	names.push_back(destkey);

	std::vector<string>::const_iterator cit = keys.begin();
	for (; cit != keys.end(); ++cit)
		names.push_back(*cit);

	const string& req = conn_.build("BITOP", NULL, names);
	return bitop(req);
}

int redis_string::bitop(const char* op, const char* destkey,
	const std::vector<const char*>& keys)
{
	std::vector<const char*> names;
	names.push_back(op);
	names.push_back(destkey);

	std::vector<const char*>::const_iterator cit = keys.begin();
	for (; cit != keys.end(); ++cit)
		names.push_back((*cit));

	const string& req = conn_.build("BITOP", NULL, names);
	return bitop(req);
}

int redis_string::bitop(const char* op, const char* destkey,
	const char* keys[], size_t size)
{
	std::vector<const char*> names;
	names.push_back(op);
	names.push_back(destkey);

	for (size_t i = 0; i < size; i++)
		names.push_back(keys[i]);

	const string& req = conn_.build("BITOP", NULL, names);
	return bitop(req);
}

int redis_string::bitop(const string& req)
{
	result_ = conn_.run(req);
	if (result_ == NULL)
		return -1;
	if (result_->get_type() != REDIS_RESULT_INTEGER)
		return -1;
	return result_->get_integer();
}

/////////////////////////////////////////////////////////////////////////////

bool redis_string::mset(const std::map<string, string>& objs)
{
	const string& req = conn_.build("MSET", NULL, objs);
	return mset(req);
}

bool redis_string::mset(const std::map<int, string>& objs)
{
	const string& req = conn_.build("MSET", NULL, objs);
	return mset(req);
}

bool redis_string::mset(const std::vector<string>& keys,
	const std::vector<string>& values)
{
	const string& req = conn_.build("MSET", NULL, keys, values);
	return mset(req);
}

bool redis_string::mset(const std::vector<int>& keys,
	const std::vector<string>& values)
{
	const string& req = conn_.build("MSET", NULL, keys, values);
	return mset(req);
}

bool redis_string::mset(const char* keys[], const char* values[], size_t argc)
{
	const string& req = conn_.build("MSET", NULL, keys, values, argc);
	return mset(req);
}

bool redis_string::mset(const char* keys[], const size_t keys_len[],
	const char* values[], const size_t values_len[], size_t argc)
{
	const string& req = conn_.build("MSET", NULL, keys, keys_len,
		values, values_len, argc);
	return mset(req);
}

bool redis_string::mset(const string& req)
{
	result_ = conn_.run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_STATUS)
		return false;
	const char* ptr = result_->get(0);
	if (ptr == NULL || strcasecmp(ptr, "OK") != 0)
		return false;
	return true;
}

/////////////////////////////////////////////////////////////////////////////

int redis_string::msetnx(const std::map<string, string>& objs)
{
	const string& req = conn_.build("MSETNX", NULL, objs);
	return msetnx(req);
}

int redis_string::msetnx(const std::map<int, string>& objs)
{
	const string& req = conn_.build("MSETNX", NULL, objs);
	return msetnx(req);
}

int redis_string::msetnx(const std::vector<string>& keys,
	const std::vector<string>& values)
{
	const string& req = conn_.build("MSETNX", NULL, keys, values);
	return msetnx(req);
}

int redis_string::msetnx(const std::vector<int>& keys,
	const std::vector<string>& values)
{
	const string& req = conn_.build("MSETNX", NULL, keys, values);
	return msetnx(req);
}


int redis_string::msetnx(const char* keys[], const char* values[], size_t argc)
{
	const string& req = conn_.build("MSETNX", NULL, keys, values, argc);
	return msetnx(req);
}

int redis_string::msetnx(const char* keys[], const size_t keys_len[],
	const char* values[], const size_t values_len[], size_t argc)
{
	const string& req = conn_.build("MSETNX", NULL, keys, keys_len,
		values, values_len, argc);
	return msetnx(req);
}

int redis_string::msetnx(const string& req)
{
	result_ = conn_.run(req);
	if (result_ == NULL)
		return -1;
	if (result_->get_type() != REDIS_RESULT_INTEGER)
		return -1;
	int ret = result_->get_integer();
	if (ret == 0)
		return 0;
	else if (ret == 1)
		return 1;
	else
		return -1;
}

/////////////////////////////////////////////////////////////////////////////

bool redis_string::mget(const std::vector<string>& keys,
	std::vector<string>* result /* = NULL */)
{
	const string& req = conn_.build("MGET", NULL, keys);
	return mget(req, result);
}

bool redis_string::mget(const std::vector<const char*>& keys,
	std::vector<string>* result /* = NULL */)
{
	const string& req = conn_.build("MGET", NULL, keys);
	return mget(req, result);
}

bool redis_string::mget(const std::vector<char*>& keys,
	std::vector<string>* result /* = NULL */)
{
	const string& req = conn_.build("MGET", NULL, keys);
	return mget(req, result);
}

bool redis_string::mget(const std::vector<int>& keys,
	std::vector<string>* result /* = NULL */)
{
	const string& req = conn_.build("MGET", NULL, keys);
	return mget(req, result);
}

bool redis_string::mget(std::vector<string>* result,
	const char* first_key, ...)
{
	std::vector<const char*> keys;
	keys.push_back(first_key);
	va_list ap;
	va_start(ap, first_key);
	const char* key;
	while ((key = va_arg(ap, const char*)) != NULL)
		keys.push_back(key);
	va_end(ap);

	return mget(keys, result);
}

bool redis_string::mget(const char* keys[], size_t argc,
	std::vector<string>* result /* = NULL */)
{
	const string& req = conn_.build("MGET", NULL, keys, argc);
	return mget(req, result);
}

bool redis_string::mget(const int keys[], size_t argc,
	std::vector<string>* result /* = NULL */)
{
	const string& req = conn_.build("MGET", NULL, keys, argc);
	return mget(req, result);
}

bool redis_string::mget(const char* keys[], const size_t keys_len[],
	size_t argc, std::vector<string>* result /* = NULL */)
{
	const string& req = conn_.build("MGET", NULL, keys, keys_len, argc);
	return mget(req, result);
}

bool redis_string::mget(const string& req,
	std::vector<string>* result /* = NULL */)
{
	result_ = conn_.run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_ARRAY)
		return false;
	if (result == NULL)
		return true;

	size_t size = mget_size();
	string buf(4096);
	const redis_result* rr;
	size_t nslice, len;
	const char* ptr;

	for (size_t i = 0; i < size; i++)
	{
		rr = mget_result(i);
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

const char* redis_string::mget_value(size_t i, size_t* len /* = NULL */) const
{
	const redis_result* rr = mget_result(i);
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

const redis_result* redis_string::mget_result(size_t i) const
{
	if (result_ == NULL)
		return NULL;
	if (result_->get_type() != REDIS_RESULT_ARRAY)
		return NULL;
	return result_->get_child(i);
}

size_t redis_string::mget_size() const
{
	if (result_ == NULL)
		return 0;
	if (result_->get_type() != REDIS_RESULT_ARRAY)
		return 0;
	return result_->get_size();
}

/////////////////////////////////////////////////////////////////////////////

bool redis_string::incr(const char* key, long long int* result /* = NULL */)
{
	return incoper("INCR", key, 1, result);
}

bool redis_string::incrby(const char* key, long long int inc,
	long long int* result /* = NULL */)
{
	return incoper("INCRBY", key, inc, result);
}

bool redis_string::incrbyfloat(const char* key, double inc,
	double* result /* = NULL */)
{
	const char* keys[1];
	keys[0] = key;
	char buf[FLOAT_LEN];
	(void) safe_snprintf(buf, sizeof(buf), "%f", inc);

	const char* values[1];
	values[0] = buf;

	const string& req = conn_.build("INCRBYFLOAT", NULL, keys, values, 1);
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

bool redis_string::decr(const char* key, long long int* result /* = NULL */)
{
	return incoper("DECR", key, 1, result);
}

bool redis_string::decrby(const char* key, long long int dec,
	long long int* result /* = NULL */)
{
	return incoper("DECRBY", key, dec, result);
}

bool redis_string::incoper(const char* cmd, const char* key, long long int n,
	long long int* result)
{
	size_t argc = 1;
	const char* names[2];

	names[0] = key;

	char buf[INT64_LEN];
	if (n != 1)
	{
		(void) acl_i64toa(n, buf, sizeof(buf));
		names[1] = buf;
		argc++;
	}

	const string& req = conn_.build(cmd, NULL, names, argc);
	result_ = conn_.run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_INTEGER)
		return false;
	if (result != NULL)
		*result = result_->get_integer64();
	return true;
}

/////////////////////////////////////////////////////////////////////////////

} // namespace acl
