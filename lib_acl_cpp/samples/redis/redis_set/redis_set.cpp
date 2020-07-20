#include "stdafx.h"

static acl::string __keypre("set_key");

static bool test_sadd(acl::redis_set& redis, int n)
{
	acl::string key;
	std::vector<acl::string> members;
	acl::string member;

	members.reserve(1000);

	for (int j = 0; j < 1000; j++)
	{
		member.format("member_%d", j);
		members.push_back(member);
	}

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);

		redis.clear();
		int ret = redis.sadd(key, members);
		if (ret < 0)
		{
			printf("sadd key: %s error\r\n", key.c_str());
			return false;
		}
		else if (i >= 10)
			continue;

		printf("sadd ok, key: %s, ret: %d\r\n", key.c_str(), ret);
	}

	return true;
}

static bool test_scard(acl::redis_set& redis, int n)
{
	acl::string key;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		int ret = redis.scard(key.c_str());
		if (ret < 0)
		{
			printf("scard key: %s error\r\n", key.c_str());
			return false;
		}
		else if (i >= 10)
			continue;

		printf("scard ok, key: %s, count: %d\r\n", key.c_str(), ret);
	}

	return true;
}

static bool test_sdiff(acl::redis_set& redis, int n)
{
	acl::string key;
	std::vector<acl::string> keys;
	std::vector<acl::string> result;

	for (int i = 0; i < 10; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		keys.push_back(key);
	}

	for (int i = 0; i < n; i++)
	{
		redis.clear();
		int ret = redis.sdiff(keys, &result);
		if (ret < 0)
		{
			printf("sdiff error\r\n");
			return false;
		}
		else if (i < 10)
			printf("sdiff ok, count: %d\r\n", ret);
	}

	return true;
}

static bool test_sdiffstore(acl::redis_set& redis, int n)
{
	acl::string key, dest_key("set_dest_key");
	std::vector<acl::string> keys;

	for (int i = 0; i < 10; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		keys.push_back(key);
	}

	for (int i = 0; i < n; i++)
	{
		redis.clear();
		int ret = redis.sdiffstore(dest_key.c_str(), keys);
		if (ret < 0)
		{
			printf("sdiffstore error, dest: %s\r\n",
				dest_key.c_str());
			return false;
		}
		else if (i >= 10)
			continue;

		printf("sdiffstore ok, dest: %s, count: %d\r\n",
			dest_key.c_str(), ret);
	}

	return true;
}

static bool test_sismember(acl::redis_set& redis, int n)
{
	bool ret;
	acl::string key, member;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);

		for (int j = 0; j < 1000; j++)
		{
			member.format("member_%d", j);
			redis.clear();
			ret = redis.sismember(key.c_str(), member.c_str());
			if (redis.eof())
			{
				printf("sismmeber eof, key: %s, member: %s\r\n",
					key.c_str(), member.c_str());
				return false;
			}
			if (i >= 10)
				continue;

			printf("sismember: %s, key: %s, member: %s\r\n",
				ret ? "true" : "false", key.c_str(),
				member.c_str());
		}
	}

	return true;
}

static bool test_smembers(acl::redis_set& redis, int n)
{
	acl::string key;
	std::vector<acl::string> members;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		int ret = redis.smembers(key.c_str(), &members);
		if (ret < 0)
		{
			printf("smembers error, key: %s\r\n", key.c_str());
			return false;
		}
		else if (i >= 10)
			continue;
	
		printf("smembers ok, key: %s, count: %d\r\n",
			key.c_str(), ret);
	}

	return true;
}

static bool test_smove(acl::redis_set& redis, int n)
{
	acl::string src_key, dst_key;
	acl::string member;
	int ret;

	for (int i = 0; i < n; i++)
	{
		src_key.format("%s_%d", __keypre.c_str(), i);
		dst_key.format("dest_%s_%d", __keypre.c_str(), i);

		for (int j = 0; j < 1000; j++)
		{
			member.format("member_%d", j);
			redis.clear();
			ret = redis.smove(src_key.c_str(), dst_key.c_str(),
					member.c_str());

			if (ret < 0)
			{
				printf("smove error, src: %s, des: %s\r\n",
					src_key.c_str(), dst_key.c_str());
				return false;
			}
			else if (j * i >= 100)
				continue;

			printf("smove ok, src: %s, dst: %s, member:%s, ret: %d\r\n",
				src_key.c_str(), dst_key.c_str(),
				member.c_str(), ret);
		}
	}

	return true;
}

static bool test_spop(acl::redis_set& redis, int n)
{
	acl::string key;
	acl::string member;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		bool ret = redis.spop(key.c_str(), member);
		if (redis.eof())
		{
			printf("spop eof, key: %s\r\n", key.c_str());
			return false;
		}
		if (i >= 10)
			continue;

		printf("spop %s, key: %s, member: %s\r\n",
			ret ? "OK" : "EMPTY", key.c_str(),
			ret ? member.c_str() : "");
	}

	return true;
}

static bool test_srandmember(acl::redis_set& redis, int n)
{
	acl::string key;
	acl::string member;
	int ret;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		ret = redis.srandmember(key.c_str(), member);
		if (redis.eof())
			return false;

		if (i >= 10)
			continue;

		printf("srandmember %s, key: %s, member: %s\r\n",
			ret ? "ok" : "empty", key.c_str(), member.c_str());
	}

	return true;
}

static bool test_srem(acl::redis_set& redis, int n)
{
	acl::string key;
	acl::string member;
	std::vector<acl::string> members;

	members.reserve(1000);

	for (int j = 0; j < 1000; j++)
	{
		member.format("member_%d", j);
		members.push_back(member);
	}

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);

		redis.clear();
		int ret = redis.srem(key.c_str(), members);

		if (ret < 0)
		{
			printf("srem error, key: %s\r\n", key.c_str());
			return false;
		}
		if (i >= 10)
			continue;

		printf("srem ok, key: %s, ret: %d\r\n", key.c_str(), ret);
	}

	return true;
}

static bool test_sunion(acl::redis_set& redis ,int n)
{
	acl::string key;
	std::vector<acl::string> keys;
	std::vector<acl::string> result;

	for (int i = 0; i < 10; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		keys.push_back(key);
	}

	for (int i = 0; i < n; i++)
	{
		redis.clear();
		int ret = redis.sunion(keys, &result);
		if (ret < 0)
		{
			printf("sunion error\r\n");
			return false;
		}
		else if (i < 10)
			printf("sunion ok, count: %d\r\n", ret);
	}

	return true;
}

static bool test_sunionstore(acl::redis_set& redis, int n)
{
	acl::string key, dest_key("set_dest_key");
	std::vector<acl::string> keys;

	for (int i = 0; i < 10; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		keys.push_back(key);
	}

	for (int i = 0; i < n; i++)
	{
		redis.clear();
		int ret = redis.sunionstore(dest_key.c_str(), keys);
		if (ret < 0)
		{
			printf("sdiffstore error, dest: %s\r\n",
				dest_key.c_str());
			return false;
		}
		else if (i < 10)
			printf("sdiffstore ok, dest: %s, count: %d\r\n",
			dest_key.c_str(), ret);
	}

	return true;
}

static bool test_sscan(acl::redis_set& redis, int n)
{
	acl::string key;
	int   ret = 0;
	std::vector<acl::string> result;
	std::vector<acl::string>::const_iterator cit;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		while (true)
		{
			redis.clear();
			ret = redis.sscan(key.c_str(), ret, result);
			if (ret < 0)
			{
				printf("sscan failed, key: %s\r\n",
					key.c_str());
				return false;
			}

			if (i >= 10)
			{
				if (ret == 0)
					break;
			}
			
			for (cit = result.begin(); cit != result.end(); ++cit)
				printf("%s: %s\r\n", key.c_str(),
					(*cit).c_str());
			if (ret == 0)
			{
				printf("sscan over, key: %s\r\n", key.c_str());
				break;
			}
		}
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
		"-a cmd[sadd|scard|sdiffstore|sismember|smembers|smove|spop|srandmember|sunion|sunionstore|sscan|srem\r\n",
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

	acl::redis_set redis;

	if (cluster_mode)
		redis.set_cluster(&cluster);
	else
		redis.set_client(&client);

	bool ret;

	if (cmd == "sadd")
		ret = test_sadd(redis, n);
	else if (cmd == "scard")
		ret = test_scard(redis, n);
	else if (cmd == "sdiff")
		ret = test_sdiff(redis, n);
	else if (cmd == "sdiffstore")
		ret = test_sdiffstore(redis, n);
	else if (cmd == "sismember")
		ret = test_sismember(redis, n);
	else if (cmd == "smembers")
		ret = test_smembers(redis, n);
	else if (cmd == "smove")
		ret = test_smove(redis, n);
	else if (cmd == "spop")
		ret = test_spop(redis, n);
	else if (cmd == "srandmember")
		ret = test_srandmember(redis, n);
	else if (cmd == "srem")
		ret = test_srem(redis, n);
	else if (cmd == "sunion")
		ret = test_sunion(redis, n);
	else if (cmd == "sunionstore")
		ret = test_sunionstore(redis, n);
	else if (cmd == "sscan")
		ret = test_sscan(redis, n);
	else if (cmd == "all")
	{
		ret = test_sadd(redis, n)
			&& test_scard(redis, n)
			&& test_sdiff(redis, n)
			&& test_sdiffstore(redis, n)
			&& test_sismember(redis, n)
			&& test_smembers(redis, n)
			&& test_smove(redis, n)
			&& test_spop(redis, n)
			&& test_srandmember(redis, n)
			&& test_sunion(redis, n)
			&& test_sunionstore(redis, n)
			&& test_sscan(redis, n)
			&& test_srem(redis, n);
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
