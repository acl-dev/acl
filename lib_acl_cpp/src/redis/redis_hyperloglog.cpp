#include "acl_stdafx.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_hyperloglog.hpp"

namespace acl
{

redis_hyperloglog::redis_hyperloglog(redis_client* conn /* = NULL */)
: redis_command(conn)
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
	const string& req = conn_->build("PFADD", key, elements);
	return conn_->get_number(req);
}

int redis_hyperloglog::pfadd(const char* key,
	const std::vector<string>& elements)
{
	const string& req = conn_->build("PFADD", key, elements);
	return conn_->get_number(req);
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
	const string& req = conn_->build("PFCOUNT", NULL, keys);
	return conn_->get_number(req);
}

int redis_hyperloglog::pfcount(const std::vector<string>& keys)
{
	const string& req = conn_->build("PFCOUNT", NULL, keys);
	return conn_->get_number(req);
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
	const string& req = conn_->build("PFMERGE", dst, keys);
	return conn_->get_status(req);
}

bool redis_hyperloglog::pfmerge(const char* dst,
	const std::vector<string>& keys)
{
	const string& req = conn_->build("PFMERGE", dst, keys);
	return conn_->get_status(req);
}

} //namespace acl
