#include "lib_acl.h"
#include "acl_cpp/memcache/memcache.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

static time_t __timeout = 100;
static const char* __key = "test";

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -l addr -g[get] -s[set] -d[del] -m[update timeout] -n num -t timeout -T nthread\n", procname);
}

static void get(const char* addr, int id, int num)
{
	acl::mem_cache client(addr);
	acl::string buf, key;
	int   i;

	client.set_prefix(__key);

	for (i = 0; i < num; i++)
	{
		key.format("key:%d:%d", id, i);
		if (client.get(key.c_str(), buf) == false)
			break;
		if (i <= 100)
			printf("GET: %s, %s\n", key.c_str(), buf.c_str());
		if (i % 10000 == 0)
		{
			char tmp[256];
			snprintf(tmp, sizeof(tmp), "GET, i: %d, num: %d", i, num);
			ACL_METER_TIME(tmp);
		}
	}
	printf("OVER: total: %d\n", i);
}

static void mod(const char* addr, int id, int num)
{
	acl::mem_cache client(addr);
	acl::string key;
	int   i;

	client.set_prefix(__key);

	for (i = 0; i < num; i++)
	{
		key.format("key:%d:%d", id, i);
		if (client.set(key.c_str(), __timeout) == false)
			break;
		if (i <= 100)
			printf("UPDATE: %s, timeout: %d\n", key.c_str(), (int) __timeout);
		if (i % 10000 == 0)
		{
			char tmp[256];
			snprintf(tmp, sizeof(tmp), "SET, i: %d, num: %d\n", i, num);
			ACL_METER_TIME(tmp);
		}
	}
	printf("OVER: total: %d\n", i);
}

static void set(const char* addr, int id, int num)
{
	acl::mem_cache client(addr);
	acl::string buf, key;
	int   i;

	client.set_prefix(__key);

	for (i = 0; i < num; i++)
	{
		key.format("key:%d:%d", id, i);
		buf.format("dat:%d, sp( ), tab(\t), eq(=);", i);
		if (client.set(key.c_str(), buf, buf.length(), __timeout) == false)
		{
			printf("SET error: %s\n", client.last_serror());
			break;
		}
		if (i <= 100)
			printf("SET: %s\n", key.c_str());
		if (i % 10000 == 0)
		{
			char tmp[256];
			snprintf(tmp, sizeof(tmp), "SET, i: %d, num: %d, timeout: %d",
				i, num, (int) __timeout);
			ACL_METER_TIME(tmp);
		}
	}
	client.property_list();
	printf("OVER: total: %d, timeout: %d\n", i, (int) __timeout);
}

static void del(const char* addr, int id, int num)
{
	acl::mem_cache client(addr);
	acl::string key;
	int   i;

	client.set_prefix(__key);

	for (i = 0; i < num; i++)
	{
		key.format("key:%d:%d", id, i);
		if (client.del(key.c_str()) == false)
			break;
		if (i <= 100)
			printf("DEL: %s\n", key.c_str());
		if (i % 10000 == 0)
		{
			char tmp[256];
			snprintf(tmp, sizeof(tmp), "DEL, i: %d, num: %d", i, num);
			ACL_METER_TIME(tmp);
		}
	}
	printf("OVER: total: %d\n", i);
}

typedef struct
{
	void (*fn)(const char* addr, int id, int num);
	int id;
	int num;
	const char* addr;
} CTX;

static void thread_main(void *arg)
{
	CTX *ctx = (CTX*) arg;

	ctx->fn(ctx->addr, ctx->id, ctx->num);
	acl_myfree(ctx);
}

static void test1(void)
{
	acl::mem_cache client("127.0.0.1:16815", 180, 300);
	acl::string buf, key("F6B7AB8F24790F6224DEC0D674FFAC4D");

	client.set_prefix("WEBMAIL").auto_retry(false).encode_key(false);

	if (client.get(key.c_str(), buf) == false)
		printf("error\r\n");
	else
		printf("ok\r\n");
	exit(0);
}

int main(int argc, char* argv[])
{
	if (0)
		test1();

	int   ch, num = 10, nthread = 1;
	char  addr[256];
	void (*func)(const char* addr, int id, int num) = NULL;

	snprintf(addr, sizeof(addr), "127.0.0.1:11211");

	while ((ch = getopt(argc, argv, "hl:gsdmn:t:T:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return (0);
		case 'l':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'g':
			func = get;
			break;
		case 's':
			func = set;
			break;
		case 'm':
			func = mod;
			break;
		case 'd':
			func = del;
			break;
		case 'n':
			num = atoi(optarg);
			break;
		case 't':
			__timeout = atoi(optarg);
			break;
		case 'T':
			nthread = atoi(optarg);
			break;
		default:
			break;
		}
	}

	printf("server: %s\n", addr);

	if (func == NULL)
		usage(argv[0]);
	else
	{
		if (nthread <= 1)
			func(addr, 0, num);
		else
		{
			acl_pthread_pool_t* pool = acl_thread_pool_create(nthread, 100);
			for (int i = 0; i < nthread; i++)
			{
				CTX* ctx = (CTX*) acl_mycalloc(1, sizeof(CTX));
				ctx->fn = func;
				ctx->addr = addr;
				ctx->num = num;
				ctx->id = i;
				acl_pthread_pool_add(pool, thread_main, ctx);
			}

			acl_pthread_pool_destroy(pool);
		}
	}
	return (0);
}
