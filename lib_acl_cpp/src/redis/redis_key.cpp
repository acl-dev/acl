#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_key.hpp"

namespace acl
{

#define INT_LEN	11

redis_key::redis_key(redis_client* conn /* = NULL */)
: redis_command(conn)
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
	const string& req = conn_->build("DEL", NULL, keys);
	return conn_->get_number(req);
}

int redis_key::del(const std::vector<char*>& keys)
{
	const string& req = conn_->build("DEL", NULL, keys);
	return conn_->get_number(req);
}

int redis_key::del(const std::vector<const char*>& keys)
{
	const string& req = conn_->build("DEL", NULL, keys);
	return conn_->get_number(req);
}

int redis_key::del(const std::vector<int>& keys)
{
	const string& req = conn_->build("DEL", NULL, keys);
	return conn_->get_number(req);
}

int redis_key::del(const char* keys[], size_t argc)
{
	const string& req = conn_->build("DEL", NULL, keys, argc);
	return conn_->get_number(req);
}

int redis_key::del(const int keys[], size_t argc)
{
	const string& req = conn_->build("DEL", NULL, keys, argc);
	return conn_->get_number(req);
}

int redis_key::del(const char* keys[], const size_t lens[], size_t argc)
{
	const string& req = conn_->build("DEL", NULL, keys, lens, argc);
	return conn_->get_number(req);
}

int redis_key::dump(const char* key, string& out)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "DUMP";
	lens[0] = sizeof("DUMP") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	const string& req = conn_->build_request(2, argv, lens);
	return conn_->get_string(req, out);
}

bool redis_key::exists(const char* key)
{
	const char* keys[1];
	keys[0] = key;

	const string& req = conn_->build("EXISTS", NULL, keys, 1);
	return conn_->get_number(req) > 0 ? true : false;
}

int redis_key::set_expire(const char* key, int n)
{
	const char* argv[2];
	argv[0]  = key;
	char buf[INT_LEN];
	(void) safe_snprintf(buf, INT_LEN, "%d", n);
	argv[1] = buf;

	const string& req = conn_->build("EXPIRE", NULL, argv, 2);
	return conn_->get_number(req);
}

int redis_key::keys_pattern(const char* pattern, std::vector<string>& out)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "KEYS";
	lens[0] = sizeof("KEYS") - 1;

	argv[1] = pattern;
	lens[1] = strlen(pattern);

	const string& req = conn_->build_request(2, argv, lens);
	return conn_->get_strings(req, out);
}

bool redis_key::rename_key(const char* key, const char* newkey)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "RENAME";
	lens[0] = sizeof("RENAME") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	argv[2] = newkey;
	lens[2] = strlen(newkey);

	const string& req = conn_->build_request(3, argv, lens);
	return conn_->get_status(req);
}

bool redis_key::renamenx(const char* key, const char* newkey)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "RENAMENX";
	lens[0] = sizeof("RENAMENX") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	argv[2] = newkey;
	lens[2] = strlen(newkey);

	const string& req = conn_->build_request(3, argv, lens);
	return conn_->get_status(req);
}

bool redis_key::restore(const char* key, const char* value, size_t len,
	int ttl, bool replace /* = false */)
{
	const char* argv[5];
	size_t lens[5];

	argv[0] = "RESTORE";
	lens[0] = sizeof("RESTORE") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	char ttl_s[INT_LEN];
	safe_snprintf(ttl_s, sizeof(ttl_s), "%d", ttl);
	argv[2] = ttl_s;
	lens[2] = strlen(ttl_s);

	argv[3] = value;
	lens[3] = len;

	size_t argc = 4;
	if (replace)
	{
		argv[4] = "REPLACE";
		lens[4] = sizeof("REPLACE") - 1;
		argc++;
	}

	const string& req = conn_->build_request(argc, argv, lens);
	return conn_->get_status(req);
}

int redis_key::get_ttl(const char* key)
{
	const char* argv[1];
	argv[0] = key;

	const string& req = conn_->build("TTL", NULL, argv, 1);

	bool success;
	int ret = conn_->get_number(req, &success);
	if (success == false)
		return -3;
	else
		return ret;
}


redis_key_t redis_key::type(const char* key)
{
	const char* keys[1];
	keys[0] = key;

	const string& req = conn_->build("TYPE", NULL, keys, 1);
	const char* ptr = conn_->get_status_string(req);
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

bool redis_key::migrate(const char* key, const char* addr, unsigned dest_db,
	unsigned timeout, const char* option /* = NULL */)
{
	char addrbuf[64];
	safe_snprintf(addrbuf, sizeof(addrbuf), "%s", addr);
	char* at = strchr(addrbuf, ':');
	if (at == NULL || *(at + 1) == 0)
		return false;
	*at++ = 0;
	int port = atoi(at);
	if (port >= 65535 || port <= 0)
		return false;

	const char* argv[7];
	size_t lens[7];
	size_t argc = 6;

	argv[0] = "MIGRATE";
	lens[0] = sizeof("MIGRATE") - 1;
	argv[1] = addrbuf;
	lens[1] = strlen(addrbuf);
	argv[2] = at;
	lens[2] = strlen(at);
	argv[3] = key;
	lens[3] = strlen(key);

	char db_s[11];
	safe_snprintf(db_s, sizeof(db_s), "%u", dest_db);
	argv[4] = db_s;
	lens[4] = strlen(db_s);

	char timeout_s[11];
	safe_snprintf(timeout_s, sizeof(timeout_s), "%u", timeout);
	argv[5] = timeout_s;
	lens[5] = strlen(timeout_s);

	if (option && *option)
	{
		argv[6] = option;
		lens[6] = strlen(option);
		argc++;
	}

	const string& req = conn_->build_request(argc, argv, lens);
	return conn_->get_status(req);
}

int redis_key::move(const char* key, unsigned dest_db)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "MOVE";
	lens[0] = sizeof("MOVE") - 1;
	argv[1] = key;
	lens[1] = strlen(key);

	char db_s[11];
	safe_snprintf(db_s, sizeof(db_s), "%u", dest_db);
	argv[2] = db_s;
	lens[2] = strlen(db_s);

	const string& req = conn_->build_request(3, argv, lens);
	return conn_->get_number(req);
}

/////////////////////////////////////////////////////////////////////////////

} // namespace acl
