#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_hash.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

#define INT64_LEN	21
#define FLOAT_LEN	32

redis_hash::redis_hash(void)
{
}

redis_hash::redis_hash(redis_client* conn)
: redis_command(conn)
{
}

redis_hash::redis_hash(redis_client_cluster* cluster)
: redis_command(cluster)
{
}

redis_hash::redis_hash(redis_client_cluster* cluster, size_t)
: redis_command(cluster)
{
}

redis_hash::~redis_hash(void)
{
}

/////////////////////////////////////////////////////////////////////////////

bool redis_hash::hmset(const char* key, const std::map<string, string>& attrs)
{
	hash_slot(key);
	build("HMSET", key, attrs);
	return check_status();
}

bool redis_hash::hmset(const char* key,size_t klen,
	const std::map<string, string>& attrs)
{
	hash_slot(key, klen);
	build("HMSET", key, klen, attrs);
	return check_status();
}

bool redis_hash::hmset(const char* key, const std::map<string, const char*>& attrs)
{
	hash_slot(key);
	build("HMSET", key, attrs);
	return check_status();
}

bool redis_hash::hmset(const char* key, const std::vector<string>& names,
	const std::vector<string>& values)
{
	acl_assert(names.size() == values.size());

	hash_slot(key);
	build("HMSET", key, names, values);
	return check_status();
}

bool redis_hash::hmset(const char* key, size_t klen,
	const std::vector<string>& names, const std::vector<string>& values)
{
	acl_assert(names.size() == values.size());

	hash_slot(key, klen);
	build("HMSET", key, klen, names, values);
	return check_status();
}

bool redis_hash::hmset(const char* key, const std::vector<const char*>& names,
	const std::vector<const char*>& values)
{
	acl_assert(names.size() == values.size());

	hash_slot(key);
	build("HMSET", key, names, values);
	return check_status();
}

bool redis_hash::hmset(const char* key, const char* names[],
	const char* values[], size_t argc)
{
	hash_slot(key);
	build("HMSET", key, names, values, argc);
	return check_status();
}

bool redis_hash::hmset(const char* key, const char* names[],
	const size_t names_len[], const char* values[],
	const size_t values_len[], size_t argc)
{
	hash_slot(key);
	build("HMSET", key, names, names_len, values, values_len, argc);
	return check_status();
}

bool redis_hash::hmset(const char* key, size_t klen, const char* names[],
	const size_t names_len[], const char* values[],
	const size_t values_len[], size_t argc)
{
	hash_slot(key, klen);
	build("HMSET", key, klen, names, names_len, values, values_len, argc);
	return check_status();
}

/////////////////////////////////////////////////////////////////////////////

bool redis_hash::hmget(const char* key, const std::vector<string>& names,
	std::vector<string>* result /* = NULL */)
{
	hash_slot(key);
	build("HMGET", key, names);
	return get_strings(result) >= 0 ? true : false;
}

bool redis_hash::hmget(const char* key, size_t klen,
	const std::vector<string>& names,
	std::vector<string>* result /* = NULL */)
{
	hash_slot(key, klen);
	build("HMGET", key, klen, names);
	return get_strings(result) >= 0 ? true : false;
}

bool redis_hash::hmget(const char* key, const std::vector<const char*>& names,
	std::vector<string>* result /* = NULL */)
{
	hash_slot(key);
	build("HMGET", key, names);
	return get_strings(result) >= 0 ? true : false;
}

bool redis_hash::hmget(const char* key, const char* names[], size_t argc,
	std::vector<string>* result /* = NULL */)
{
	hash_slot(key);
	build("HMGET", key, names, argc);
	return get_strings(result) >= 0 ? true : false;
}

bool redis_hash::hmget(const char* key, const char* names[],
	const size_t lens[], size_t argc, std::vector<string>* result /* = NULL */)
{
	hash_slot(key);
	build("HMGET", key, names, lens, argc);
	return get_strings(result) >= 0 ? true : false;
}

bool redis_hash::hmget(const char* key, size_t klen, const char* names[],
	const size_t lens[], size_t argc, std::vector<string>* result /* = NULL */)
{
	hash_slot(key, klen);
	build("HMGET", key, klen, names, lens, argc);
	return get_strings(result) >= 0 ? true : false;
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
	return hset(key, strlen(key), name, name_len, value, value_len);
}

int redis_hash::hset(const char* key, size_t klen, const char* name,
	size_t name_len, const char* value, size_t value_len)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "HSET";
	lens[0] = sizeof("HSET") - 1;
	argv[1] = key;
	lens[1] = klen;
	argv[2] = name;
	lens[2] = name_len;
	argv[3] = value;
	lens[3] = value_len;

	hash_slot(key, klen);
	build_request(4, argv, lens);
	return get_number();
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
	return hsetnx(key, strlen(key), name, name_len, value, value_len);
}

int redis_hash::hsetnx(const char* key, size_t klen, const char* name,
	size_t name_len, const char* value, size_t value_len)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "HSETNX";
	lens[0] = sizeof("HSETNX") - 1;
	argv[1] = key;
	lens[1] = klen;
	argv[2] = name;
	lens[2] = name_len;
	argv[3] = value;
	lens[3] = value_len;

	hash_slot(key, klen);
	build_request(4, argv, lens);
	return get_number();
}

bool redis_hash::hget(const char* key, const char* name, string& result)
{
	return hget(key, name, strlen(name), result);
}

bool redis_hash::hget(const char* key, const char* name,
	size_t name_len, string& result)
{
	return hget(key, strlen(key), name, name_len, result);
}

bool redis_hash::hget(const char* key, size_t klen, const char* name,
	size_t name_len, string& result)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "HGET";
	lens[0] = sizeof("HGET") - 1;
	argv[1] = key;
	lens[1] = klen;
	argv[2] = name;
	lens[2] = name_len;

	hash_slot(key, klen);
	build_request(3, argv, lens);
	return get_string(result) >= 0 ? true : false;
}

bool redis_hash::hgetall(const char* key, std::map<string, string>& result)
{
	return hgetall(key, strlen(key), result);
}

bool redis_hash::hgetall(const char* key, size_t klen,
	std::map<string, string>& result)
{
	const char* keys[1];
	keys[0] = key;
	size_t klens[1];
	klens[0] = klen;

	hash_slot(key, klen);
	build("HGETALL", NULL, keys, klens, 1);
	return get_strings(result) < 0 ? false : true;
}

bool redis_hash::hgetall(const char* key, std::vector<string>& names,
	std::vector<string>& values)
{
	const char* keys[1];
	keys[0] = key;

	hash_slot(key);
	build("HGETALL", NULL, keys, 1);
	return get_strings(names, values) < 0 ? false : true;
}

bool redis_hash::hgetall(const char* key, std::vector<const char*>& names,
	std::vector<const char*>& values)
{
	const char* keys[1];
	keys[0] = key;

	hash_slot(key);
	build("HGETALL", NULL, keys, 1);
	return get_strings(names, values) < 0 ? false : true;
}

int redis_hash::hdel(const char* key, const char* name)
{
	return hdel_fields(key, name, NULL);
}

int redis_hash::hdel_fields(const char* key, const char* first_name, ...)
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
	return hdel_fields(key, names, argc);
}

int redis_hash::hdel_fields(const char* key, const char* names[], size_t argc)
{
	hash_slot(key);
	build("HDEL", key, names, argc);
	return get_number();
}

int redis_hash::hdel(const char* key, const char* names[],
	const size_t names_len[], size_t argc)
{
	return hdel(key, strlen(key), names, names_len, argc);
}

int redis_hash::hdel(const char* key, size_t klen,
	const char* names[], const size_t names_len[], size_t argc)
{
	return hdel_fields(key, klen, names, names_len, argc);
}

int redis_hash::hdel_fields(const char* key, const char* names[],
	const size_t names_len[], size_t argc)
{
	return hdel_fields(key, strlen(key), names, names_len, argc);
}

int redis_hash::hdel_fields(const char* key, size_t klen,
	const char* names[], const size_t names_len[], size_t argc)
{
	hash_slot(key, klen);
	build("HDEL", key, klen, names, names_len, argc);
	return get_number();;
}

int redis_hash::hdel(const char* key, const std::vector<string>& names)
{
	return hdel(key, strlen(key), names);
}

int redis_hash::hdel(const char* key, size_t klen,
	const std::vector<string>& names)
{
	return hdel_fields(key, klen, names);
}

int redis_hash::hdel_fields(const char* key, const std::vector<string>& names)
{
	return hdel_fields(key, strlen(key), names);
}

int redis_hash::hdel_fields(const char* key, size_t klen,
	const std::vector<string>& names)
{
	hash_slot(key, klen);
	build("HDEL", key, klen, names);
	return get_number();
}

int redis_hash::hdel(const char* key, const std::vector<const char*>& names)
{
	return hdel_fields(key, names);
}

int redis_hash::hdel_fields(const char* key, const std::vector<const char*>& names)
{
	hash_slot(key);
	build("HDEL", key, names);
	return get_number();
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

	hash_slot(key);
	build("HINCRBY", key, names, values, 1);

	bool success;

	if (result != NULL)
		*result = get_number64(&success);
	else
		(void) get_number64(&success);

	return success;
}

bool redis_hash::hincrbyfloat(const char* key, const char* name,
	double inc, double* result /* = NULL */)
{
	const char* names[1];
	const char* values[1];

	names[0] = name;
	char buf[FLOAT_LEN];
	(void) safe_snprintf(buf, sizeof(buf), "%f", inc);
	values[0] = buf;

	hash_slot(key);
	build("HINCRBYFLOAT", key, names, values, 1);
	if (get_string(buf, sizeof(buf)) <= 0)
		return false;

	if (result != NULL)
		*result = atof(buf);
	return true;
}

/////////////////////////////////////////////////////////////////////////////

bool redis_hash::hkeys(const char* key, std::vector<string>& names)
{
	return hkeys(key, strlen(key), names);
}

bool redis_hash::hkeys(const char* key, size_t klen, std::vector<string>& names)
{
	const char* keys[1];
	keys[0] = key;

	size_t lens[1];
	lens[0] = klen;

	hash_slot(key, klen);
	build("HKEYS", NULL, keys, lens, 1);
	return get_strings(names) < 0 ? false : true;
}

bool redis_hash::hexists(const char* key, const char* name)
{
	return hexists(key, name, strlen(name));
}

bool redis_hash::hexists(const char* key, const char* name, size_t name_len)
{
	return hexists(key, strlen(key), name, name_len);
}

bool redis_hash::hexists(const char* key, size_t klen,
	const char* name, size_t name_len)
{
	const char* names[1];
	names[0] = name;
	size_t names_len[1];
	names_len[0] = name_len;

	hash_slot(key, klen);
	build("HEXISTS", key, klen, names, names_len, 1);
	return get_number() > 0 ? true : false;
}

int redis_hash::hlen(const char* key)
{
	return hlen(key, strlen(key));
}

int redis_hash::hlen(const char* key, size_t klen)
{
	const char* keys[1];
	keys[0] = key;
	size_t klens[1];
	klens[0] = klen;

	hash_slot(key, klen);
	build("HLEN", NULL, keys, klens, 1);
	return get_number();
}

int redis_hash::hstrlen(const char* key, const char* name)
{
	return hstrlen(key, name, strlen(name));
}

int redis_hash::hstrlen(const char* key, const char* name, size_t name_len)
{
	return hstrlen(key, strlen(key), name, name_len);
}

int redis_hash::hstrlen(const char* key, size_t klen,
	const char* name, size_t name_len)
{
	const char* names[1];
	names[0] = name;
	size_t names_len[1];
	names_len[0] = name_len;

	hash_slot(key, klen);
	build("HSTRLEN", key, klen, names, names_len, 1);
	return get_number();
}

bool redis_hash::hvals(const char* key, std::vector<string>& values)
{
	return hvals(key, strlen(key), values);
}

bool redis_hash::hvals(const char* key, size_t klen,
	std::vector<string>& values)
{
	const char* keys[1];
	keys[0] = key;

	size_t klens[1];
	klens[0] = klen;

	hash_slot(key);
	build("HVALS", NULL, keys, klens, 1);
	return get_strings(values) >= 0 ? true : false;
}

int redis_hash::hscan(const char* key, int cursor,
	std::map<string, string>& out, const char* pattern /* = NULL */,
	const size_t* count /* = NULL */)
{
	return hscan(key, strlen(key), cursor, out, pattern, count);
}

int redis_hash::hscan(const char* key, size_t klen, int cursor,
	std::map<string, string>& out, const char* pattern /* = NULL */,
	const size_t* count /* = NULL */)
{
	if (key == NULL || *key == 0 || klen == 0 || cursor < 0)
		return -1;

	size_t size;
	const redis_result** children = scan_keys("HSCAN", key, klen, cursor,
		size, pattern, count);
	if (children == NULL)
		return cursor;

	if (size % 2 != 0)
		return -1;

	const redis_result* rr;
	string name(128), value(128);
	// out.clear();

	for (size_t i = 0; i < size;) {
		rr = children[i];
		rr->argv_to_string(name);
		i++;

		rr = children[i];
		rr->argv_to_string(value);
		i++;

		out[name] = value;
	}

	return cursor;
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
