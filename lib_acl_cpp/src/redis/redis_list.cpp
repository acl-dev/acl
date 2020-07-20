#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_list.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

#define INT_LEN		11
#define LONG_LEN	21

redis_list::redis_list()
{
}

redis_list::redis_list(redis_client* conn)
: redis_command(conn)
{
}

redis_list::redis_list(redis_client_cluster* cluster)
: redis_command(cluster)
{
}

redis_list::redis_list(redis_client_cluster* cluster, size_t)
: redis_command(cluster)
{
}

redis_list::~redis_list()
{
}

//////////////////////////////////////////////////////////////////////////

int redis_list::llen(const char* key)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "LLEN";
	lens[0] = sizeof("LLEN") - 1;
	argv[1] = key;
	lens[1] = strlen(key);

	hash_slot(key);
	build_request(2, argv, lens);
	return get_number();
}

bool redis_list::lindex(const char* key, size_t idx, string& buf)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "LINDEX";
	lens[0] = sizeof("LINDEX") - 1;
	argv[1] = key;
	lens[1] = strlen(key);

	char tmp[LONG_LEN];
	(void) safe_snprintf(tmp, sizeof(tmp), "%lu", (unsigned long) idx);
	argv[2] = tmp;
	lens[2] = strlen(tmp);

	hash_slot(key);
	build_request(3, argv, lens);
	return get_string(buf) >= 0 ? true : false;
}

bool redis_list::lset(const char* key, int idx, const char* value)
{
	return lset(key, idx, value, strlen(value));
}

bool redis_list::lset(const char* key, int idx, const char* value, size_t len)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "LSET";
	lens[0] = sizeof("LSET") - 1;
	argv[1] = key;
	lens[1] = strlen(key);

	char tmp[LONG_LEN];
	(void) safe_snprintf(tmp, sizeof(tmp), "%lu", (unsigned long) idx);
	argv[2] = tmp;
	lens[2] = strlen(tmp);

	argv[3] = value;
	lens[3] = len;

	hash_slot(key);
	build_request(4, argv, lens);
	return check_status();
}

int redis_list::linsert_before(const char* key, const char* pivot,
	const char* value)
{
	return linsert_before(key, pivot, strlen(pivot), value, strlen(value));
}

int redis_list::linsert_before(const char* key, const char* pivot,
	size_t pivot_len, const char* value, size_t value_len)
{
	return linsert(key, "BEFORE", pivot, pivot_len, value, value_len);
}

int redis_list::linsert_after(const char* key, const char* pivot,
	const char* value)
{
	return linsert_after(key, pivot, strlen(pivot), value, strlen(value));
}

int redis_list::linsert_after(const char* key, const char* pivot,
	size_t pivot_len, const char* value, size_t value_len)
{
	return linsert(key, "AFTER", pivot, pivot_len, value, value_len);
}

int redis_list::linsert(const char* key, const char* pos, const char* pivot,
	size_t pivot_len, const char* value, size_t value_len)
{
	const char* argv[5];
	size_t lens[5];

	argv[0] = "LINSERT";
	lens[0] = sizeof("LINSERT") - 1;
	argv[1] = key;
	lens[1] = strlen(key);
	argv[2] = pos;
	lens[2] = strlen(pos);
	argv[3] = pivot;
	lens[3] = pivot_len;
	argv[4] = value;
	lens[4] = value_len;

	hash_slot(key);
	build_request(5, argv, lens);
	return get_number();
}

int redis_list::lpush(const char* key, const char* first_value, ...)
{
	std::vector<const char*> values;
	values.push_back(first_value);

	va_list ap;
	va_start(ap, first_value);
	const char* value;

	while ((value = va_arg(ap, const char*)) != NULL)
		values.push_back(value);
	va_end(ap);

	return lpush(key, values);
}

int redis_list::lpush(const char* key, const char* values[], size_t argc)
{
	hash_slot(key);
	build("LPUSH", key, values, argc);
	return get_number();
}

int redis_list::lpush(const char* key, const std::vector<string>& values)
{
	hash_slot(key);
	build("LPUSH", key, values);
	return get_number();
}

int redis_list::lpush(const char* key, const std::vector<const char*>& values)
{
	hash_slot(key);
	build("LPUSH", key, values);
	return get_number();
}

int redis_list::lpush(const char* key, const char* values[],
	const size_t lens[], size_t argc)
{
	hash_slot(key);
	build("LPUSH", key, values, lens, argc);
	return get_number();
}

int redis_list::rpush(const char* key, const char* first_value, ...)
{
	std::vector<const char*> values;
	values.push_back(first_value);

	va_list ap;
	va_start(ap, first_value);
	const char* value;

	while ((value = va_arg(ap, const char*)) != NULL)
		values.push_back(value);
	va_end(ap);

	return rpush(key, values);
}

int redis_list::rpush(const char* key, const char* values[], size_t argc)
{
	hash_slot(key);
	build("RPUSH", key, values, argc);
	return get_number();
}

int redis_list::rpush(const char* key, const std::vector<string>& values)
{
	hash_slot(key);
	build("RPUSH", key, values);
	return get_number();
}

int redis_list::rpush(const char* key, const std::vector<const char*>& values)
{
	hash_slot(key);
	build("RPUSH", key, values);
	return get_number();
}

int redis_list::rpush(const char* key, const char* values[],
	const size_t lens[], size_t argc)
{
	hash_slot(key);
	build("RPUSH", key, values, lens, argc);
	return get_number();
}

int redis_list::lpushx(const char* key, const char* value)
{
	return lpushx(key, value, strlen(value));
}

int redis_list::lpushx(const char* key, const char* value, size_t len)
{
	return pushx("LPUSHX", key, value, len);
}

int redis_list::rpushx(const char* key, const char* value)
{
	return rpushx(key, value, strlen(value));
}

int redis_list::rpushx(const char* key, const char* value, size_t len)
{
	return pushx("RPUSHX", key, value, len);
}

int redis_list::pushx(const char* cmd, const char* key,
	const char* value, size_t len)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = cmd;
	lens[0] = strlen(cmd);
	argv[1] = key;
	lens[1] = strlen(key);
	argv[2] = value;
	lens[2] = len;

	hash_slot(key);
	build_request(3, argv, lens);
	return get_number();
}

//////////////////////////////////////////////////////////////////////////

int redis_list::lpop(const char* key, string& buf)
{
	return pop("LPOP", key, buf);
}

int redis_list::rpop(const char* key, string& buf)
{
	return pop("RPOP", key, buf);
}

int redis_list::pop(const char* cmd, const char* key, string& buf)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = cmd;
	lens[0] = strlen(cmd);
	argv[1] = key;
	lens[1] = strlen(key);

	hash_slot(key);
	build_request(2, argv, lens);
	return (int) get_string(buf);
}

bool redis_list::blpop(std::pair<string, string>& result, size_t timeout,
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

	hash_slot(first_key);
	return blpop(keys, timeout, result);
}

bool redis_list::blpop(const std::vector<const char*>& keys, size_t timeout,
	std::pair<string, string>& result)
{
	return bpop("BLPOP", keys, timeout, result);
}


bool redis_list::blpop(const std::vector<string>& keys, size_t timeout,
	std::pair<string, string>& result)
{
	return bpop("BLPOP", keys, timeout, result);
}

bool redis_list::brpop(std::pair<string, string>& result, size_t timeout,
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

	hash_slot(first_key);
	return brpop(keys, timeout, result);
}

bool redis_list::brpop(const std::vector<const char*>& keys, size_t timeout,
	std::pair<string, string>& result)
{
	return bpop("BRPOP", keys, timeout, result);
}


bool redis_list::brpop(const std::vector<string>& keys, size_t timeout,
	std::pair<string, string>& result)
{
	return bpop("BRPOP", keys, timeout, result);
}

bool redis_list::bpop(const char* cmd, const std::vector<const char*>& keys,
	size_t timeout, std::pair<string, string>& result)
{
	size_t argc = 2 + keys.size();
	const char** args = (const char**) dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	args[0] = cmd;
	lens[0] = strlen(cmd);

	size_t i = 1;
	std::vector<const char*>::const_iterator cit = keys.begin();
	for (; cit != keys.end(); ++cit) {
		args[i] = *cit;
		lens[i] = strlen(args[i]);
		i++;
	}

	char buf[LONG_LEN];
	safe_snprintf(buf, sizeof(buf), "%lu", (unsigned long) timeout);
	args[i] = buf;
	lens[i] = strlen(args[i]);

	build_request(argc, args, lens);
	return bpop(result);
}

bool redis_list::bpop(const char* cmd, const std::vector<string>& keys,
	size_t timeout, std::pair<string, string>& result)
{
	size_t argc = 2 + keys.size();
	const char** args = (const char**) dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	args[0] = cmd;
	lens[0] = strlen(cmd);

	size_t i = 1;
	std::vector<string>::const_iterator cit = keys.begin();
	for (; cit != keys.end(); ++cit) {
		args[i] = (*cit).c_str();
		lens[i] = (*cit).length();
		i++;
	}

	char buf[LONG_LEN];
	safe_snprintf(buf, sizeof(buf), "%lu", (unsigned long) timeout);
	args[i] = buf;
	lens[i] = strlen(args[i]);

	build_request(argc, args, lens);
	return bpop(result);
}

bool redis_list::bpop(std::pair<string, string>& out)
{
	int rw_timeout = -1;
	const redis_result* result = run(0, &rw_timeout);
	if (result == NULL)
		return false;
	if (result->get_type() != REDIS_RESULT_ARRAY)
		return false;
	size_t size = result->get_size();
	if (size == 0)
		return false;
	if (size != 2)
		return false;

	const redis_result* first = result->get_child(0);
	const redis_result* second = result->get_child(1);
	if (first == NULL || second == NULL
		|| first->get_type() != REDIS_RESULT_STRING
		|| second->get_type() != REDIS_RESULT_STRING) {

		return false;
	}

	string buf;
	first->argv_to_string(buf);
	out.first = buf;

	second->argv_to_string(buf);
	out.second = buf;
	return true;
}

bool redis_list::rpoplpush(const char* src, const char* dst,
	string* buf /* = NULL */)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "RPOPLPUSH";
	lens[0] = sizeof("RPOPLPUSH") - 1;
	argv[1] = src;
	lens[1] = strlen(src);
	argv[2] = dst;
	lens[2] = strlen(dst);

	build_request(3, argv, lens);
	return get_string(buf) >= 0 ? true : false;
}

bool redis_list::brpoplpush(const char* src, const char* dst,
	size_t timeout, string* buf /* = NULL */)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "BRPOPLPUSH";
	lens[0] = sizeof("BRPOPLPUSH") - 1;
	argv[1] = src;
	lens[1] = strlen(src);
	argv[2] = dst;
	lens[2] = strlen(dst);

	char tmp[LONG_LEN];
	safe_snprintf(tmp, sizeof(tmp), "%lu", (unsigned long) timeout);
	argv[3] = tmp;
	lens[3] = strlen(argv[3]);

	build_request(4, argv, lens);
	return get_string(buf) >= 0 ? true : false;
}

bool redis_list::lrange(const char* key, int start, int end,
	std::vector<string>* result)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "LRANGE";
	lens[0] = sizeof("LRANGE") - 1;
	argv[1] = key;
	lens[1] = strlen(key);

	char start_s[LONG_LEN], end_s[LONG_LEN];
	safe_snprintf(start_s, sizeof(start_s), "%d", start);
	safe_snprintf(end_s, sizeof(end_s), "%d", end);

	argv[2] = start_s;
	lens[2] = strlen(start_s);
	argv[3] = end_s;
	lens[3] = strlen(end_s);

	hash_slot(key);
	build_request(4, argv, lens);
	return get_strings(result) < 0 ? false : true;
}

int redis_list::lrem(const char* key, int count, const char* value)
{
	return lrem(key, count, value, strlen(value));
}

int redis_list::lrem(const char* key, int count, const char* value, size_t len)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "LREM";
	lens[0] = sizeof("LREM") - 1;
	argv[1] = key;
	lens[1] = strlen(key);

	char buf[INT_LEN];
	safe_snprintf(buf, sizeof(buf), "%d", count);
	argv[2] = buf;
	lens[2] = strlen(buf);

	argv[3] = value;
	lens[3] = len;

	hash_slot(key);
	build_request(4, argv, lens);
	return get_number();
}

bool redis_list::ltrim(const char* key, int start, int end)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "LTRIM";
	lens[0] = sizeof("LTRIM") - 1;
	argv[1] = key;
	lens[1] = strlen(key);

	char start_s[LONG_LEN], end_s[LONG_LEN];
	safe_snprintf(start_s, sizeof(start_s), "%d", start);
	safe_snprintf(end_s, sizeof(end_s), "%d", end);

	argv[2] = start_s;
	lens[2] = strlen(start_s);
	argv[3] = end_s;
	lens[3] = strlen(end_s);

	hash_slot(key);
	build_request(4, argv, lens);
	return check_status();
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
