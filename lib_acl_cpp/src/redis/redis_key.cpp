#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_key.hpp"

namespace acl
{

#define INT_LEN	11

redis_key::redis_key(redis_client& conn)
: conn_(conn)
{

}

redis_key::~redis_key()
{

}

/////////////////////////////////////////////////////////////////////////////

int redis_key::del(const char* first_key, ...)
{
	std::vector<const char*> keys;

	keys.push_back(first_key);
	const char* key;
	va_list ap;
	va_start(ap, first_key);
	while ((key = va_arg(ap, const char*)) != NULL)
		keys.push_back(key);
	va_end(ap);
	return del(keys);
}

int redis_key::del(const std::vector<string>& keys)
{
	const string& req = conn_.build("DEL", NULL, keys);
	return del(req);
}

int redis_key::del(const std::vector<char*>& keys)
{
	const string& req = conn_.build("DEL", NULL, keys);
	return del(req);
}

int redis_key::del(const std::vector<const char*>& keys)
{
	const string& req = conn_.build("DEL", NULL, keys);
	return del(req);
}

int redis_key::del(const std::vector<int>& keys)
{
	const string& req = conn_.build("DEL", NULL, keys);
	return del(req);
}

int redis_key::del(const char* keys[], size_t argc)
{
	const string& req = conn_.build("DEL", NULL, keys, argc);
	return del(req);
}

int redis_key::del(const int keys[], size_t argc)
{
	const string& req = conn_.build("DEL", NULL, keys, argc);
	return del(req);
}

int redis_key::del(const char* keys[], const size_t lens[], size_t argc)
{
	const string& req = conn_.build("DEL", NULL, keys, lens, argc);
	return del(req);
}

int redis_key::del(const string& req)
{
	result_ = conn_.run(req);
	return result_ == NULL ? -1 : result_->get_integer();
}

/////////////////////////////////////////////////////////////////////////////

int redis_key::expire(const char* key, int n)
{
	const char* argv[2];
	argv[0]  = key;
	char buf[INT_LEN];
	(void) safe_snprintf(buf, INT_LEN, "%d", n);
	argv[1] = buf;

	const string& req = conn_.build("EXPIRE", NULL, argv, 2);
	const redis_result* rr = conn_.run(req);
	if (rr == NULL)
		return -1;
	return rr->get_integer();
}

int redis_key::ttl(const char* key)
{
	const char* argv[1];
	argv[0] = key;

	const string& req = conn_.build("TTL", NULL, argv, 1);
	const redis_result* rr = conn_.run(req);
	if (rr == NULL)
		return -1;
	bool success;
	int ret = rr->get_integer(&success);
	if (!success)
		return -1;
	if (ret < 0)
		return 0;
	return ret;
}

bool redis_key::exists(const char* key)
{
	const char* keys[1];
	keys[0] = key;

	const string& req = conn_.build("EXISTS", NULL, keys, 1);
	const redis_result* rr = conn_.run(req);
	if (rr == NULL)
		return false;
	return rr->get_integer() > 0 ? true : false;
}

redis_key_t redis_key::type(const char* key)
{
	const char* keys[1];
	keys[0] = key;

	const string& req = conn_.build("TYPE", NULL, keys, 1);
	const redis_result* rr = conn_.run(req);
	if (rr == NULL)
	{
		logger_error("result null");
		return REDIS_KEY_UNKNOWN;
	}

	const char* ptr = rr->get_status();
	if (ptr == NULL)
		return REDIS_KEY_UNKNOWN;

	if (strcasecmp(ptr, "none") == 0)
		return REDIS_KEY_NONE;
	else if (strcasecmp(ptr, "string") == 0)
		return REDIS_KEY_STRING;
	else if (strcasecmp(ptr, "list") == 0)
		return REDIS_KEY_LIST;
	else if (strcasecmp(ptr, "set") == 0)
		return REDIS_KEY_SET;
	else if (strcasecmp(ptr, "zset") == 0)
		return REDIS_KEY_ZSET;
	else
	{
		logger_error("unknown type: %s, key: %s", ptr, key);
		return REDIS_KEY_UNKNOWN;
	}
}

/////////////////////////////////////////////////////////////////////////////

} // namespace acl
