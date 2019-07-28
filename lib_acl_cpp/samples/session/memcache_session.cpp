// session.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

bool test_memcache_session(const char* addr, int n)
{
	acl::memcache_session sess(addr);

	char  name[32], value[32];

	printf(">>>>>>>>>>>>> set session <<<<<<<<<<<<<<<<<<<\r\n");
	for (int i = 0; i < n; i++)
	{
		acl::safe_snprintf(name, sizeof(name), "name-%d", i);
		acl::safe_snprintf(value, sizeof(value), "value-%d", i);
		if (sess.set(name, value) == 0)
		{
			printf("set error, name: %s, value: %s\r\n", name, value);
			return false;
		}
		printf("set ok, name: %s, value: %s\r\n", name, value);
	}

	printf("\r\n>>>>>>>>>>>>> get session <<<<<<<<<<<<<<<<<<<\r\n");
	for (int i = 0; i < n; i++)
	{
		acl::safe_snprintf(name, sizeof(name), "name-%d", i);
		acl::safe_snprintf(value, sizeof(value), "value-%d", i);
		const char* ptr = sess.get(name);
		if (ptr == NULL || *ptr == 0 || strcmp(ptr, value) != 0)
		{
			printf("get error, name: %s\r\n", name);
			return false;
		}
		printf("get ok, name: %s, value: %s\r\n", name, ptr);
	}

	printf("\r\n>>>>>>>>>>>>> del session <<<<<<<<<<<<<<<<<<<\r\n");
	for (int i = 0; i < n; i++)
	{
		acl::safe_snprintf(name, sizeof(name), "name-%d", i);
		if (sess.del(name) == false)
		{
			printf("del error, name: %s\r\n", name);
			return false;
		}
		printf("del ok, name: %s\r\n", name);
	}

	printf("\r\n>>>>>>>>>>>>> get session <<<<<<<<<<<<<<<<<<<\r\n");
	for (int i = 0; i < n; i++)
	{
		acl::safe_snprintf(name, sizeof(name), "name-%d", i);
		const char* ptr = sess.get(name);
		if (ptr == NULL || *ptr == 0)
			printf("get ok, name: %s no exist\r\n", name);
		else
		{
			printf("get error, name: %s exist, value: %s\r\n",
				name, value);
			return false;
		}
	}

	printf("\r\n------------ test session ok now -------------\r\n");
	return true;
}

void test_memcache_session_delay(const char* addr)
{
	const char* sid = "XXXXXXXXXXXXXX";
	acl::memcache_session sess(addr, 120, 300, NULL, 0, sid);
	sess.set_ttl(128, true);

	char name[128], value[128];
	for (int i = 0; i < 10; i++)
	{
		acl::safe_snprintf(name, sizeof(name), "name%d", i);
		acl::safe_snprintf(value, sizeof(value), "value%d", i);
		sess.set(name, value);
		printf(">>>set %s: %s\r\n", name, value);
	}

	acl::safe_snprintf(name, sizeof(name), "name5");
	sess.del_delay(name);

	if (sess.flush() == false)
	{
		printf("set session error\r\n");
		return;
	}

	printf("set session ok\r\n");
	printf("begin get session:\r\n");

	for (int i = 0; i < 11; i++)
	{
		acl::safe_snprintf(name, sizeof(name), "name%d", i);
		acl::safe_snprintf(value, sizeof(value), "value%d", i);
		const char* ptr = sess.get(name);
		if (ptr == NULL)
			printf(">>> %s not found\r\n", name);
		else
			printf(">>>get %s: %s, %s\r\n", name, ptr,
				strcmp(ptr, value) == 0 ? "ok" : "failed");
	}
}
