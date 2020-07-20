#include "stdafx.h"

static acl::string __keypre("test_key");

static bool test_del(acl::redis_key& redis, int n)
{
	acl::string key;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		int ret = redis.del_one(key.c_str());
		if (ret < 0)
		{
			printf("del key: %s error: %s\r\n",
				key.c_str(), redis.result_error());
			return false;
		}
		else if (i < 10)
			printf("del ok, key: %s, ret: %d\r\n", key.c_str(), ret);
	}

	return true;
}

static bool test_expire(acl::redis_key& redis, int n)
{
	acl::string key;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		if (redis.expire(key.c_str(), 100) < 0)
		{
			printf("expire key: %s error: %s\r\n",
				key.c_str(), redis.result_error());
			return false;
		}
		else if (i < 10)
			printf("expire ok, key: %s\r\n", key.c_str());
	}

	return true;
}

static bool test_ttl(acl::redis_key& redis, int n)
{
	acl::string key;
	int ttl;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		if ((ttl = redis.ttl(key.c_str())) < 0)
		{
			printf("get ttl key: %s error: %s\r\n",
				key.c_str(), redis.result_error());
			return false;
		}
		else if (i < 10)
			printf("ttl ok, key: %s, ttl: %d\r\n",
				key.c_str(), ttl);
	}

	return true;
}

static bool test_exists(acl::redis_key& redis, int n)
{
	acl::string key;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		if (redis.exists(key.c_str()) == false)
			printf("no exists key: %s\r\n", key.c_str());
		else
			printf("exists key: %s\r\n", key.c_str());
	}

	return true;
}

static bool test_type(acl::redis_key& redis, int n)
{
	acl::string key;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		acl::redis_key_t ret = redis.type(key.c_str());
		if (ret == acl::REDIS_KEY_NONE)
		{
			printf("unknown type key: %s\r\n", key.c_str());
			return false;
		}
		else
			printf("type ok, key: %s, ret: %d\r\n",
				key.c_str(), ret);
	}

	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s redis_addr[127.0.0.1:6379]\r\n"
		"-n count\r\n"
		"-C connect_timeout[default: 10]\r\n"
		"-T rw_timeout[default: 10]\r\n"
		"-c [use cluster mode]\r\n"
		"-a cmd[del|expire|ttl|exists|type|all]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 1, conn_timeout = 10, rw_timeout = 10;
	acl::string addr("127.0.0.1:6379"), cmd;
	bool cluster_mode = false;

	while ((ch = getopt(argc, argv, "hs:n:C:T:a:c")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'n':
			n = atoi(optarg);
			break;
		case 'C':
			conn_timeout = atoi(optarg);
			break;
		case 'T':
			rw_timeout = atoi(optarg);
			break;
		case 'a':
			cmd = optarg;
			break;
		case 'c':
			cluster_mode = true;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();

	acl::redis_client_cluster cluster;
	cluster.set(addr.c_str(), 100, conn_timeout, rw_timeout);

	acl::redis_client client(addr.c_str(), conn_timeout, rw_timeout);

	acl::redis_key redis;

	if (cluster_mode)
		redis.set_cluster(&cluster);
	else
		redis.set_client(&client);

	bool ret;

	if (cmd == "del")
		ret = test_del(redis, n);
	else if (cmd == "expire")
		ret = test_expire(redis, n);
	else if (cmd == "ttl")
		ret = test_ttl(redis, n);
	else if (cmd == "exists")
		ret = test_exists(redis, n);
	else if (cmd == "type")
		ret = test_type(redis, n);
	else if (cmd == "all")
	{
		ret = test_expire(redis, n)
			&& test_ttl(redis, n)
			&& test_exists(redis, n)
			&& test_type(redis, n)
			&& test_del(redis, n);
	}
	else
	{
		ret = false;
		printf("unknown cmd: %s\r\n", cmd.c_str());
	}

	if (ret == true)
		printf("test OK!\r\n");
	else
		printf("test failed!\r\n");

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
