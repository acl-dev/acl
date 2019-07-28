// session.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "session.h"

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"-s server_addr\r\n"
		"-n count\r\n"
		"-c max_threads\r\n"
		"-a action[memcache|memcache_delay|redis]\r\n", procname);
}

int main(int argc, char* argv[])
{
	char  addr[256], action[32];
	int   nloop = 10, ch, max_threads = 1;

	acl::acl_cpp_init();

	acl::safe_snprintf(action, sizeof(action), "redis");
	acl::safe_snprintf(addr, sizeof(addr), "192.168.0.250:11211");

	while ((ch = getopt(argc, argv, "hs:n:c:a:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			acl::safe_snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'n':
			nloop = atoi(optarg);
			if (nloop <= 0)
				nloop = 10;
			break;
		case 'c':
			max_threads = atoi(optarg);
			break;
		case 'a':
			acl::safe_snprintf(action, sizeof(action), "%s", optarg);
			break;
		default:
			break;
		}
	}

	printf("nloop: %d, max_threads: %d\r\n", nloop, max_threads);

	if (strcasecmp(action, "memcache") == 0)
		test_memcache_session(addr, nloop);
	else if (strcasecmp(action, "memcache_delay") == 0)
		test_memcache_session_delay(addr);
	else if (strcasecmp(action, "redis") == 0)
		test_redis_session(addr, nloop, max_threads);
	else if (strcasecmp(action, "redis_attrs") == 0)
		test_redis_session_attrs(addr, nloop);
	else
		printf("unknown action: %s\r\n", action);

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}

