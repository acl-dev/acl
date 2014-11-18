#pragma once
#include "acl_cpp/stdlib/string.hpp"

typedef enum
{
	REDIS_KEY_NULL,
	REDIS_KEY_STR,
	REDIS_KEY_LIST,
	REDIS_KEY_SET,
	REDIS_KEY_ZSET,
	REDIS_KEY_HASH
} redis_key_t;

namespace acl
{

class redis_response
{
public:
	redis_response() {}
	~redis_response() {}

	bool isString()
	{
		return type == REDIS_KEY_STR;
	}

private:
	redis_key_t type;
	unsigned count;
	union
	{
		string* value;
		long long intval;
		redis_response* values;
	};
};

} // end namespace acl
