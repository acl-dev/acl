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

redis_string::redis_string(redis_client* conn /* = NULL */)
: redis_command(conn)
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
	const char* argv[3];
	size_t lens[3];

	argv[0] = "SET";
	lens[0] = sizeof("SET") - 1;

	argv[1] = key;
	lens[1] = key_len;

	argv[2] = value;
	lens[2] = value_len;

	conn_->build_request(3, argv, lens);
	return conn_->get_status();
}

bool redis_string::setex(const char* key, const char* value, int timeout)
{
	return setex(key, strlen(key), value, strlen(value), timeout);
}

bool redis_string::setex(const char* key, size_t key_len, const char* value,
	size_t value_len, int timeout)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "SETEX";
	lens[0] = sizeof("SETEX") - 1;

	argv[1] = key;
	lens[1] = key_len;

	char buf[INT_LEN];
	(void) safe_snprintf(buf, sizeof(buf), "%d", timeout);
	argv[2] = buf;
	lens[2] = strlen(buf);

	argv[3] = value;
	lens[3] = value_len;

	conn_->build_request(4, argv, lens);
	return conn_->get_status();
}

bool redis_string::psetex(const char* key, const char* value, int timeout)
{
	return psetex(key, strlen(key), value, strlen(value), timeout);
}

bool redis_string::psetex(const char* key, size_t key_len, const char* value,
	size_t value_len, int timeout)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "PSETEX";
	lens[0] = sizeof("PSETEX") - 1;

	argv[1] = key;
	lens[1] = key_len;

	char buf[INT_LEN];
	(void) safe_snprintf(buf, sizeof(buf), "%d", timeout);
	argv[2] = buf;
	lens[2] = strlen(buf);

	argv[3] = value;
	lens[3] = value_len;

	conn_->build_request(4, argv, lens);
	return conn_->get_status();
}

int redis_string::setnx(const char* key, const char* value)
{
	return setnx(key, strlen(key), value, strlen(value));
}

int redis_string::setnx(const char* key, size_t key_len,
	const char* value, size_t value_len)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "SETNX";
	lens[0] = sizeof("SETNX") - 1;

	argv[1] = key;
	lens[1] = key_len;

	argv[2] = value;
	lens[2] = value_len;

	conn_->build_request(3, argv, lens);
	return conn_->get_number();
}

int redis_string::append(const char* key, const char* value)
{
	return append(key, value, strlen(value));
}

int redis_string::append(const char* key, const char* value, size_t size)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "APPEND";
	lens[0] = sizeof("APPEND") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	argv[2] = value;
	lens[2] = size;

	conn_->build_request(3, argv, lens);
	return conn_->get_number();
}

bool redis_string::get(const char* key, string& buf)
{
	return get(key, strlen(key), buf);
}

bool redis_string::get(const char* key, size_t len, string& buf)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "GET";
	lens[0] = sizeof("GET") - 1;

	argv[1] = key;
	lens[1] = len;

	conn_->build_request(2, argv, lens);
	return conn_->get_string(buf) >= 0 ? true : false;
}

const redis_result* redis_string::get(const char* key)
{
	return get(key, strlen(key));
}

const redis_result* redis_string::get(const char* key, size_t len)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "GET";
	lens[0] = sizeof("GET") - 1;

	argv[1] = key;
	lens[1] = len;

	conn_->build_request(2, argv, lens);
	const redis_result* result = conn_->run();
	if (result == NULL)
		return NULL;
	if (result->get_type() != REDIS_RESULT_STRING)
		return NULL;
	return result;
}

bool redis_string::getset(const char* key, const char* value, string& buf)
{
	return getset(key, strlen(key), value, strlen(value), buf);
}

bool redis_string::getset(const char* key, size_t key_len,
	const char* value, size_t value_len, string& buf)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "GETSET";
	lens[0] = sizeof("GETSET") - 1;

	argv[1] = key;
	lens[1] = key_len;

	argv[2] = value;
	lens[2] = value_len;

	conn_->build_request(3, argv, lens);
	return conn_->get_string(buf) >= 0 ? true : false;
}

/////////////////////////////////////////////////////////////////////////////

int redis_string::get_strlen(const char* key)
{
	return get_strlen(key, strlen(key));
}

int redis_string::get_strlen(const char* key, size_t len)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "STRLEN";
	lens[0] = sizeof("STRLEN") - 1;

	argv[1] = key;
	lens[1] = len;

	conn_->build_request(2, argv, lens);
	return conn_->get_number();
}

int redis_string::setrange(const char* key, unsigned offset, const char* value)
{
	return setrange(key, strlen(key), offset, value, strlen(value));
}

int redis_string::setrange(const char* key, size_t key_len, unsigned offset,
	const char* value, size_t value_len)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "SETRANGE";
	lens[0] = sizeof("SETRANGE") - 1;

	argv[1] = key;
	lens[1] = key_len;

	char buf[INT64_LEN];
	(void) acl_i64toa(offset, buf, sizeof(buf));
	argv[2] = buf;
	lens[2] = strlen(buf);

	argv[3] = value;
	lens[3] = value_len;

	conn_->build_request(4, argv, lens);
	return conn_->get_number();
}

bool redis_string::getrange(const char* key, int start, int end, string& buf)
{
	return getrange(key, strlen(key), start, end, buf);
}

bool redis_string::getrange(const char* key, size_t key_len,
	int start, int end, string& buf)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "GETRANGE";
	lens[0] = sizeof("GETRANGE") - 1;

	argv[1] = key;
	lens[1] = key_len;

	char start_buf[INT_LEN], end_buf[INT_LEN];
	(void) safe_snprintf(start_buf, sizeof(start_buf), "%d", start);
	argv[2] = start_buf;
	lens[2] = strlen(start_buf);

	(void) safe_snprintf(end_buf, sizeof(end_buf), "%d", end);
	argv[3] = end_buf;
	lens[3] = strlen(end_buf);

	conn_->build_request(4, argv, lens);
	return conn_->get_string(buf) >= 0 ? true : false;
}

/////////////////////////////////////////////////////////////////////////////

bool redis_string::setbit(const char* key, unsigned offset, bool bit)
{
	return setbit(key, strlen(key), offset, bit);
}

bool redis_string::setbit(const char* key, size_t len,
	unsigned offset, bool bit)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "SETBIT";
	lens[0] = sizeof("SETBIT") - 1;

	argv[1] = key;
	lens[1] = len;

	char buf4off[INT_LEN];
	(void) safe_snprintf(buf4off, sizeof(buf4off), "%d", offset);
	argv[2] = buf4off;
	lens[2] = strlen(buf4off);

	argv[3] = bit ? "1" : "0";
	lens[3] = 1;

	conn_->build_request(4, argv, lens);
	return conn_->get_number() >= 0 ? true : false;
}

bool redis_string::getbit(const char* key, unsigned offset, int& bit)
{
	return getbit(key, strlen(key), offset, bit);
}

bool redis_string::getbit(const char* key, size_t len,
	unsigned offset, int& bit)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "GETBIT";
	lens[0] = sizeof("GETBIT") - 1;

	argv[1] = key;
	lens[1] = len;

	char buf4off[INT_LEN];
	(void) safe_snprintf(buf4off, sizeof(buf4off), "%d", offset);
	argv[2] = buf4off;
	lens[2] = strlen(buf4off);

	conn_->build_request(3, argv, lens);
	int ret = conn_->get_number();
	if (ret < 0)
		return false;
	bit = ret == 0 ? 0 : 1;
	return true;
}

int redis_string::bitcount(const char* key)
{
	return bitcount(key, strlen(key));
}

int redis_string::bitcount(const char* key, size_t len)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "BITCOUNT";
	lens[0] = sizeof("BITCOUNT") - 1;

	argv[1] = key;
	lens[1] = len;

	conn_->build_request(2, argv, lens);
	return conn_->get_number();
}

int redis_string::bitcount(const char* key, int start, int end)
{
	return bitcount(key, strlen(key), start, end);
}

int redis_string::bitcount(const char* key, size_t len, int start, int end)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "BITCOUNT";
	lens[0] = sizeof("BITCOUNT") - 1;

	argv[1] = key;
	lens[1] = len;

	char buf4start[INT_LEN];
	(void) safe_snprintf(buf4start, sizeof(buf4start), "%d", start);
	argv[2] = buf4start;
	lens[2] = strlen(buf4start);

	char buf4end[INT_LEN];
	(void) safe_snprintf(buf4end, sizeof(buf4end), "%d", end);
	argv[3] = buf4end;
	lens[3] = strlen(buf4end);

	conn_->build_request(4, argv, lens);
	return conn_->get_number();
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
	size_t argc = 3 + keys.size();
	dbuf_pool* pool = conn_->get_pool();
	const char** argv = (const char**) pool->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) pool->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = "BITOP";
	lens[0] = sizeof("BITOP") - 1;

	argv[1] = op;
	lens[1] = strlen(op);

	argv[2] = destkey;
	lens[2] = strlen(destkey);

	std::vector<string>::const_iterator cit = keys.begin();
	for (size_t i = 3; cit != keys.end(); ++cit, i++)
	{
		argv[i] = (*cit).c_str();
		lens[i] = strlen(argv[i]);
	}

	conn_->build_request(argc, argv, lens);
	return conn_->get_number();
}

int redis_string::bitop(const char* op, const char* destkey,
	const std::vector<const char*>& keys)
{
	size_t argc = 3 + keys.size();
	dbuf_pool* pool = conn_->get_pool();
	const char** argv = (const char**) pool->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) pool->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = "BITOP";
	lens[0] = sizeof("BITOP") - 1;

	argv[1] = op;
	lens[1] = strlen(op);

	argv[2] = destkey;
	lens[2] = strlen(destkey);

	std::vector<const char*>::const_iterator cit = keys.begin();
	for (size_t i = 3; cit != keys.end(); ++cit, i++)
	{
		argv[i] = *cit;
		lens[i] = strlen(argv[i]);
	}

	conn_->build_request(argc, argv, lens);
	return conn_->get_number();
}

int redis_string::bitop(const char* op, const char* destkey,
	const char* keys[], size_t size)
{
	size_t argc = 3 + size;
	dbuf_pool* pool = conn_->get_pool();
	const char** argv = (const char**) pool->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) pool->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = "BITOP";
	lens[0] = sizeof("BITOP") - 1;

	argv[1] = op;
	lens[1] = strlen(op);

	argv[2] = destkey;
	lens[2] = strlen(destkey);

	for (size_t i = 3, j = 0; j < size; i++, j++)
	{
		argv[i] = keys[j];
		lens[i] = strlen(argv[i]);
	}

	conn_->build_request(argc, argv, lens);
	return conn_->get_number();
}

/////////////////////////////////////////////////////////////////////////////

bool redis_string::mset(const std::map<string, string>& objs)
{
	conn_->build("MSET", NULL, objs);
	return conn_->get_status();
}

bool redis_string::mset(const std::map<int, string>& objs)
{
	conn_->build("MSET", NULL, objs);
	return conn_->get_status();
}

bool redis_string::mset(const std::vector<string>& keys,
	const std::vector<string>& values)
{
	conn_->build("MSET", NULL, keys, values);
	return conn_->get_status();
}

bool redis_string::mset(const std::vector<int>& keys,
	const std::vector<string>& values)
{
	conn_->build("MSET", NULL, keys, values);
	return conn_->get_status();
}

bool redis_string::mset(const char* keys[], const char* values[], size_t argc)
{
	conn_->build("MSET", NULL, keys, values, argc);
	return conn_->get_status();
}

bool redis_string::mset(const char* keys[], const size_t keys_len[],
	const char* values[], const size_t values_len[], size_t argc)
{
	conn_->build("MSET", NULL, keys, keys_len, values, values_len, argc);
	return conn_->get_status();
}

/////////////////////////////////////////////////////////////////////////////

int redis_string::msetnx(const std::map<string, string>& objs)
{
	conn_->build("MSETNX", NULL, objs);
	return conn_->get_number();
}

int redis_string::msetnx(const std::map<int, string>& objs)
{
	conn_->build("MSETNX", NULL, objs);
	return conn_->get_number();
}

int redis_string::msetnx(const std::vector<string>& keys,
	const std::vector<string>& values)
{
	conn_->build("MSETNX", NULL, keys, values);
	return conn_->get_number();
}

int redis_string::msetnx(const std::vector<int>& keys,
	const std::vector<string>& values)
{
	conn_->build("MSETNX", NULL, keys, values);
	return conn_->get_number();
}

int redis_string::msetnx(const char* keys[], const char* values[], size_t argc)
{
	conn_->build("MSETNX", NULL, keys, values, argc);
	return conn_->get_number();
}

int redis_string::msetnx(const char* keys[], const size_t keys_len[],
	const char* values[], const size_t values_len[], size_t argc)
{
	conn_->build("MSETNX", NULL, keys, keys_len, values, values_len, argc);
	return conn_->get_number();
}

/////////////////////////////////////////////////////////////////////////////

bool redis_string::mget(const std::vector<string>& keys,
	std::vector<string>* out /* = NULL */)
{
	conn_->build("MGET", NULL, keys);
	return conn_->get_strings(out) >= 0 ? true : false;
}

bool redis_string::mget(const std::vector<const char*>& keys,
	std::vector<string>* out /* = NULL */)
{
	conn_->build("MGET", NULL, keys);
	return conn_->get_strings(out) >= 0 ? true : false;
}

bool redis_string::mget(const std::vector<int>& keys,
	std::vector<string>* out /* = NULL */)
{
	conn_->build("MGET", NULL, keys);
	return conn_->get_strings(out) >= 0 ? true : false;
}

bool redis_string::mget(std::vector<string>* out, const char* first_key, ...)
{
	std::vector<const char*> keys;
	keys.push_back(first_key);
	va_list ap;
	va_start(ap, first_key);
	const char* key;
	while ((key = va_arg(ap, const char*)) != NULL)
		keys.push_back(key);
	va_end(ap);

	conn_->build("MGET", NULL, keys);
	return conn_->get_strings(out) >= 0 ? true : false;
}

bool redis_string::mget(const char* keys[], size_t argc,
	std::vector<string>* out /* = NULL */)
{
	conn_->build("MGET", NULL, keys, argc);
	return conn_->get_strings(out) >= 0 ? true : false;
}

bool redis_string::mget(const int keys[], size_t argc,
	std::vector<string>* out /* = NULL */)
{
	conn_->build("MGET", NULL, keys, argc);
	return conn_->get_strings(out) >= 0 ? true : false;
}

bool redis_string::mget(const char* keys[], const size_t keys_len[],
	size_t argc, std::vector<string>* out /* = NULL */)
{
	conn_->build("MGET", NULL, keys, keys_len, argc);
	return conn_->get_strings(out) >= 0 ? true : false;
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
	const char* argv[3];
	size_t lens[3];

	argv[0] = "INCRBYFLOAT";
	lens[0] = sizeof("INCRBYFLOAT") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	char buf[FLOAT_LEN];
	(void) safe_snprintf(buf, sizeof(buf), "%f", inc);
	argv[2] = buf;
	lens[2] = strlen(buf);

	conn_->build_request(3, argv, lens);
	if (conn_->get_string(buf, sizeof(buf)) == false)
		return false;

	if (result != NULL)
		*result = atof(buf);
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
	size_t argc = 2;
	const char* argv[3];
	size_t lens[3];

	argv[0] = cmd;
	lens[0] = strlen(cmd);

	argv[1] = key;
	lens[1] = strlen(key);

	char buf[INT64_LEN];
	if (n != 1)
	{
		(void) acl_i64toa(n, buf, sizeof(buf));
		argv[2] = buf;
		lens[2] = strlen(buf);
		argc++;
	}

	conn_->build_request(argc, argv, lens);

	bool success;
	if (result != NULL)
		*result = conn_->get_number64(&success);
	else
		(void) conn_->get_number64(&success);
	return success;
}

/////////////////////////////////////////////////////////////////////////////

} // namespace acl
