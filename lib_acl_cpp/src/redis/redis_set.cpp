#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_set.hpp"
#endif

namespace acl
{

#define LONG_LEN	21

redis_set::redis_set()
: redis_command(NULL)
{
}

redis_set::redis_set(redis_client* conn)
: redis_command(conn)
{
}

redis_set::redis_set(redis_client_cluster* cluster, size_t max_conns)
: redis_command(cluster, max_conns)
{
}

redis_set::~redis_set()
{
}

int redis_set::sadd(const char* key, const char* first_member, ...)
{
	std::vector<const char*> members;
	members.push_back(first_member);

	va_list ap;
	va_start(ap, first_member);
	const char* member;
	while ((member = va_arg(ap, const char*)) != NULL)
		members.push_back(member);
	va_end(ap);

	return sadd(key, members);
}

int redis_set::sadd(const char* key, const std::vector<const char*>& memsbers)
{
	hash_slot(key);
	build("SADD", key, memsbers);
	return get_number();

}

int redis_set::sadd(const char* key, const std::vector<string>& members)
{
	hash_slot(key);
	build("SADD", key, members);
	return get_number();
}

int redis_set::sadd(const char* key, const char* argv[], size_t argc)
{
	hash_slot(key);
	build("SADD", key, argv, argc);
	return get_number();
}

int redis_set::sadd(const char* key, const char* argv[],
	const size_t lens[], size_t argc)
{
	hash_slot(key);
	build("SADD", key, argv, lens, argc);
	return get_number();
}

bool redis_set::spop(const char* key, string& buf)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "SPOP";
	lens[0] = sizeof("SPOP") - 1;
	argv[1] = key;
	lens[1] = strlen(key);

	hash_slot(key);
	build_request(2, argv, lens);
	return get_string(buf) < 0 ? false : true;
}

int redis_set::scard(const char* key)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "SCARD";
	lens[0] = sizeof("SCARD") - 1;
	argv[1] = key;
	lens[1] = strlen(key);

	hash_slot(key);
	build_request(2, argv, lens);
	return get_number();
}

int redis_set::smembers(const char* key, std::vector<string>* members)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "SMEMBERS";
	lens[0] = sizeof("SMEMBERS") - 1;
	argv[1] = key;
	lens[1] = strlen(key);

	hash_slot(key);
	build_request(2, argv, lens);
	return get_strings(members);
}

int redis_set::smove(const char* src, const char* dst, const char* member)
{
	return smove(src, dst, member, strlen(member));
}

int redis_set::smove(const char* src, const char* dst, const string& member)
{
	return smove(src, dst, member.c_str(), member.length());
}

int redis_set::smove(const char* src, const char* dst, const char* member,
	size_t len)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "SMOVE";
	lens[0] = sizeof("SMOVE") - 1;
	argv[1] = src;
	lens[1] = strlen(src);
	argv[2] = dst;
	lens[2] = strlen(dst);
	argv[3] = member;
	lens[3] = len;

	build_request(4, argv, lens);
	return get_number();
}

int redis_set::sdiff(std::vector<string>* members, const char* first_key, ...)
{
	std::vector<const char*> keys;
	keys.push_back(first_key);

	va_list ap;
	va_start(ap, first_key);
	const char* key;
	while ((key = va_arg(ap, const char*)) != NULL)
		keys.push_back(key);
	va_end(ap);

	return sdiff(keys, members);
}

int redis_set::sdiff(const std::vector<const char*>& keys,
	std::vector<string>* members)
{
	build("SDIFF", NULL, keys);
	return get_strings(members);
}

int redis_set::sdiff(const std::vector<string>& keys,
	std::vector<string>* members)
{
	build("SDIFF", NULL, keys);
	return get_strings(members);
}

int redis_set::sinter(std::vector<string>* members, const char* first_key, ...)
{
	std::vector<const char*> keys;
	keys.push_back(first_key);

	va_list ap;
	va_start(ap, first_key);
	const char* key;
	while ((key = va_arg(ap, const char*)) != NULL)
		keys.push_back(key);
	va_end(ap);

	return sinter(keys, members);
}

int redis_set::sinter(const std::vector<const char*>& keys,
	std::vector<string>* members)
{
	build("SINTER", NULL, keys);
	return get_strings(members);
}

int redis_set::sinter(const std::vector<string>& keys,
	std::vector<string>* members)
{
	build("SINTER", NULL, keys);
	return get_strings(members);
}

int redis_set::sunion(std::vector<string>* members, const char* first_key, ...)
{
	std::vector<const char*> keys;
	keys.push_back(first_key);

	va_list ap;
	va_start(ap, first_key);
	const char* key;
	while ((key = va_arg(ap, const char*)) != NULL)
		keys.push_back(key);
	va_end(ap);

	return sunion(keys, members);
}

int redis_set::sunion(const std::vector<const char*>& keys,
	std::vector<string>* members)
{
	build("SUNION", NULL, keys);
	return get_strings(members);
}

int redis_set::sunion(const std::vector<string>& keys,
	std::vector<string>* members)
{
	build("SUNION", NULL, keys);
	return get_strings(members);
}

int redis_set::sdiffstore(const char* dst, const char* first_key, ...)
{
	std::vector<const char*> keys;
	keys.push_back(first_key);

	va_list ap;
	va_start(ap, first_key);
	const char* key;
	while ((key = va_arg(ap, const char*)) != NULL)
		keys.push_back(key);
	va_end(ap);

	return sdiffstore(dst, keys);
}

int redis_set::sdiffstore(const char* dst, const std::vector<const char*>& keys)
{
	build("SDIFFSTORE", dst, keys);
	return get_number();
}

int redis_set::sdiffstore(const char* dst, const std::vector<string>& keys)
{
	build("SDIFFSTORE", dst, keys);
	return get_number();
}

int redis_set::sinterstore(const char* dst, const char* first_key, ...)
{
	std::vector<const char*> keys;
	keys.push_back(first_key);

	va_list ap;
	va_start(ap, first_key);
	const char* key;
	while ((key = va_arg(ap, const char*)) != NULL)
		keys.push_back(key);
	va_end(ap);

	return sinterstore(dst, keys);
}

int redis_set::sinterstore(const char* dst, const std::vector<const char*>& keys)
{
	build("SINTERSTORE", dst, keys);
	return get_number();
}

int redis_set::sinterstore(const char* dst, const std::vector<string>& keys)
{
	build("SINTERSTORE", dst, keys);
	return get_number();
}

int redis_set::sunionstore(const char* dst, const char* first_key, ...)
{
	std::vector<const char*> keys;
	keys.push_back(first_key);

	va_list ap;
	va_start(ap, first_key);
	const char* key;
	while ((key = va_arg(ap, const char*)) != NULL)
		keys.push_back(key);
	va_end(ap);

	return sunionstore(dst, keys);
}

int redis_set::sunionstore(const char* dst, const std::vector<const char*>& keys)
{
	build("SUNIONSTORE", dst, keys);
	return get_number();
}

int redis_set::sunionstore(const char* dst, const std::vector<string>& keys)
{
	build("SUNIONSTORE", dst, keys);
	return get_number();
}

bool redis_set::sismember(const char* key, const char* member)
{
	return sismember(key, member, strlen(member));
}

bool redis_set::sismember(const char* key, const char* member, size_t len)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "SISMEMBER";
	lens[0] = sizeof("SISMEMBER") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	argv[2] = member;
	lens[2] = len;

	hash_slot(key);
	build_request(3, argv, lens);
	return get_number() > 0 ? true : false;
}

int redis_set::srandmember(const char* key, string& out)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "SRANDMEMBER";
	lens[0] = sizeof("SRANDMEMBER") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	hash_slot(key);
	build_request(2, argv, lens);
	return get_string(out);
}

int redis_set::srandmember(const char* key, size_t n, std::vector<string>& out)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "SRANDMEMBER";
	lens[0] = sizeof("SRANDMEMBER") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	char buf[LONG_LEN];
	safe_snprintf(buf, sizeof(buf), "%lu", (unsigned long) n);
	argv[2] = buf;
	lens[2] = strlen(buf);

	hash_slot(key);
	build_request(3, argv, lens);
	return get_strings(out);
}

int redis_set::srem(const char* key, const char* first_member, ...)
{
	std::vector<const char*> members;
	members.push_back(first_member);

	va_list ap;
	va_start(ap, first_member);
	const char* member;
	while ((member = va_arg(ap, const char*)) != NULL)
		members.push_back(member);
	va_end(ap);

	return srem(key, members);
}

int redis_set::srem(const char* key, const std::vector<string>& members)
{
	hash_slot(key);
	build("SREM", key, members);
	return get_number();
}

int redis_set::srem(const char* key, const std::vector<const char*>& members)
{
	hash_slot(key);
	build("SREM", key, members);
	return get_number();
}

int redis_set::srem(const char* key, const char* members[],
	size_t lens[], size_t argc)
{
	hash_slot(key);
	build("SREM", key, members, lens, argc);
	return get_number();
}

int redis_set::sscan(const char* key, int cursor, std::vector<string>& out,
	const char* pattern /* = NULL */, const size_t* count /* = NULL */)
{
	if (key == NULL || *key == 0 || cursor < 0)
		return -1;

	size_t size;
	const redis_result** children = scan_keys("SSCAN", key, cursor,
		size, pattern, count);
	if (children == NULL)
		return cursor;

	const redis_result* rr;
	string key_buf(128);

	out.reserve(out.size() + size);

	for (size_t i = 0; i < size; i++)
	{
		rr = children[i];
		rr->argv_to_string(key_buf);
		out.push_back(key_buf);
		key_buf.clear();
	}

	return cursor;
}

} // namespace acl
