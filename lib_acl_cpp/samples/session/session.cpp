// session.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "acl_cpp/acl_cpp_init.hpp"
#include "acl_cpp/session/memcache_session.hpp"

using namespace acl;

#ifdef WIN32
#define snprintf _snprintf
#endif

static int session_test1(const char* addr, int n)
{
	memcache_session s(addr);

	char  name[32], value[32];

	printf(">>>>>>>>>>>>> set session <<<<<<<<<<<<<<<<<<<\r\n");
	for (int i = 0; i < n; i++)
	{
		snprintf(name, sizeof(name), "name-%d", i);
		snprintf(value, sizeof(value), "value-%d", i);
		if (s.set(name, value) == 0)
		{
			printf("set error, name: %s, value: %s\r\n", name, value);
			return 1;
		}
		printf("set ok, name: %s, value: %s\r\n", name, value);
	}

	printf("\r\n>>>>>>>>>>>>> get session <<<<<<<<<<<<<<<<<<<\r\n");
	for (int i = 0; i < n; i++)
	{
		snprintf(name, sizeof(name), "name-%d", i);
		snprintf(value, sizeof(value), "value-%d", i);
		const char* ptr = s.get(name);
		if (ptr == NULL || *ptr == 0 || strcmp(ptr, value) != 0)
		{
			printf("get error, name: %s\r\n", name);
			return 1;
		}
		printf("get ok, name: %s, value: %s\r\n", name, ptr);
	}

	printf("\r\n>>>>>>>>>>>>> del session <<<<<<<<<<<<<<<<<<<\r\n");
	for (int i = 0; i < n; i++)
	{
		snprintf(name, sizeof(name), "name-%d", i);
		if (s.del(name) == false)
		{
			printf("del error, name: %s\r\n", name);
			return 1;
		}
		printf("del ok, name: %s\r\n", name);
	}

	printf("\r\n>>>>>>>>>>>>> get session <<<<<<<<<<<<<<<<<<<\r\n");
	for (int i = 0; i < n; i++)
	{
		snprintf(name, sizeof(name), "name-%d", i);
		const char* ptr = s.get(name);
		if (ptr == NULL || *ptr == 0)
			printf("get ok, name: %s no exist\r\n", name);
		else
		{
			printf("get error, name: %s exist, value: %s\r\n",
				name, value);
			return 1;
		}
	}

	printf("\r\n------------ test session ok now -------------\r\n");
	return 0;
}

static void session_delay_test(const char* addr)
{
	const char* sid = "XXXXXXXXXXXXXX";
	memcache_session sess(addr, 120, 300, NULL, 0, sid);
	sess.set_ttl(128, true);

	char name[128], value[128];
	for (int i = 0; i < 10; i++)
	{
		snprintf(name, sizeof(name), "name%d", i);
		snprintf(value, sizeof(value), "value%d", i);
		sess.set(name, value, true);
		printf(">>>set %s: %s\r\n", name, value);
	}

	snprintf(name, sizeof(name), "name5");
	sess.del(name, true);

	if (sess.flush() == false)
		printf("set session error\r\n");
	else
	{
		printf("set session ok\r\n");
		printf("begin get session:\r\n");

		for (int i = 0; i < 11; i++)
		{
			snprintf(name, sizeof(name), "name%d", i);
			snprintf(value, sizeof(value), "value%d", i);
			const char* ptr = sess.get(name);
			if (ptr == NULL)
				printf(">>> %s not found\r\n", name);
			else
				printf(">>>get %s: %s, %s\r\n", name, ptr,
					strcmp(ptr, value) == 0 ? "ok" : "failed");
		}
	}
}


int main(int argc, char* argv[])
{
	char  addr[256];
	int   n = 10;

	if (argc >= 2)
	{
		snprintf(addr, sizeof(addr), "%s", argv[1]);
		if (argc >= 3)
			n = atoi(argv[2]);
		if (n <= 0)
			n = 10;
	}
	else
	{
		snprintf(addr, sizeof(addr), "192.168.0.250:11211");
		n = 10;
	}

	acl_cpp_init();

	session_test1(addr, n);
	session_delay_test(addr);

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}

