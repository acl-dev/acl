#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_hyperloglog.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

redis_hyperloglog::redis_hyperloglog()
{
}

redis_hyperloglog::redis_hyperloglog(redis_client* conn)
: redis_command(conn)
{
}

redis_hyperloglog::redis_hyperloglog(redis_client_cluster* cluster)
: redis_command(cluster)
{
}

redis_hyperloglog::redis_hyperloglog(redis_client_cluster* cluster, size_t)
: redis_command(cluster)
{
}

redis_hyperloglog::~redis_hyperloglog()
{
}

int redis_hyperloglog::pfadd(const char* key, const char* first_element, ...)
{
	std::vector<const char*> elements;
	elements.push_back(first_element);

	va_list ap;
	va_start(ap, first_element);
	const char* element;
	while ((element = va_arg(ap, const char*)) != NULL)
		elements.push_back(element);
	va_end(ap);

	return pfadd(key, elements);
}

int redis_hyperloglog::pfadd(const char* key,
	const std::vector<const char*>& elements)
{
	build("PFADD", key, elements);
	return get_number();
}

int redis_hyperloglog::pfadd(const char* key,
	const std::vector<string>& elements)
{
	build("PFADD", key, elements);
	return get_number();
}

int redis_hyperloglog::pfcount(const char* first_key, ...)
{
	std::vector<const char*> keys;
	keys.push_back(first_key);

	va_list ap;
	va_start(ap, first_key);
	const char* key;
	while ((key = va_arg(ap, const char*)) != NULL)
		keys.push_back(key);
	va_end(ap);

	return pfcount(keys);
}

int redis_hyperloglog::pfcount(const std::vector<const char*>& keys)
{
	build("PFCOUNT", NULL, keys);
	return get_number();
}

int redis_hyperloglog::pfcount(const std::vector<string>& keys)
{
	build("PFCOUNT", NULL, keys);
	return get_number();
}

bool redis_hyperloglog::pfmerge(const char* dst, const char* first_src, ...)
{
	std::vector<const char*> keys;
	keys.push_back(first_src);

	va_list ap;
	va_start(ap, first_src);
	const char* src;
	while ((src = va_arg(ap, const char*)) != NULL)
		keys.push_back(src);
	va_end(ap);

	return pfmerge(dst, keys);
}

bool redis_hyperloglog::pfmerge(const char* dst,
	const std::vector<const char*>& keys)
{
	build("PFMERGE", dst, keys);
	return check_status();
}

bool redis_hyperloglog::pfmerge(const char* dst,
	const std::vector<string>& keys)
{
	build("PFMERGE", dst, keys);
	return check_status();
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
