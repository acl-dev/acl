#include "acl_stdafx.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

#define INT_LEN		11
#define	LONG_LEN	21

redis_command::redis_command(redis_client* conn /* = NULL */)
: conn_(conn)
{

}

redis_command::~redis_command()
{

}

void redis_command::reset()
{
	if (conn_)
		conn_->reset();
}

void redis_command::set_client(redis_client* conn)
{
	conn_ = conn;
}

bool redis_command::eof() const
{
	return conn_ == NULL ? false : conn_->eof();
}

const redis_result* redis_command::get_result() const
{
	return conn_ ? conn_->get_result() : NULL;
}

const redis_result** redis_command::scan_keys(const char* cmd, const char* key,
	int& cursor, size_t& size, const char* pattern, const size_t* count)
{
	size = 0;
	if (cursor < 0)
		return NULL;

	const char* argv[7];
	size_t lens[7];
	size_t argc = 0;

	argv[argc] = cmd;
	lens[argc] = strlen(cmd);
	argc++;

	if (key && *key)
	{
		argv[argc] = key;
		lens[argc] = strlen(key);
		argc++;
	}

	char cursor_s[INT_LEN];
	safe_snprintf(cursor_s, sizeof(cursor_s), "%d", cursor);
	argv[argc] = cursor_s;
	lens[argc] = strlen(cursor_s);
	argc++;

	if (pattern && *pattern)
	{
		argv[argc] = "MATCH";
		lens[argc] = sizeof("MATCH") - 1;
		argc++;

		argv[argc] = pattern;
		lens[argc] = strlen(pattern);
		argc++;
	}

	if (count && *count > 0)
	{
		argv[argc] = "COUNT";
		lens[argc] = sizeof("COUNT") - 1;
		argc++;

		char count_s[LONG_LEN];
		safe_snprintf(count_s, sizeof(count_s), "%lu",
			(unsigned long) count);
		argv[argc] = count_s;
		lens[argc] = strlen(count_s);
		argc++;
	}

	conn_->build_request(argc, argv, lens);
	const redis_result* result = conn_->run();
	if (result == NULL)
	{
		cursor = -1;
		return NULL;
	}

	if (result->get_size() != 2)
	{
		cursor = -1;
		return NULL;
	}

	const redis_result* rr = result->get_child(0);
	if (rr == NULL)
	{
		cursor = -1;
		return NULL;
	}
	string tmp(128);
	if (rr->argv_to_string(tmp) < 0)
	{
		cursor = -1;
		return NULL;
	}
	cursor = atoi(tmp.c_str());
	if (cursor < 0)
	{
		cursor = -1;
		return NULL;
	}

	rr = result->get_child(1);
	if (rr == NULL)
	{
		cursor = -1;
		return NULL;
	}

	const redis_result** children = rr->get_children(&size);
	if (children == NULL)
	{
		cursor = 0;
		size = 0;
	}

	return children;
}

} // namespace acl
