#include "stdafx.h"

static acl::string __keypre("zset_key");

static bool test_zadd(acl::redis_zset& redis, int n)
{
	acl::string key;
	std::map<acl::string, double> members;
	acl::string member;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);

		for (int j = 0; j < 1000; j++)
		{
			member.format("member_%d", j);
			members[member] = j;
		}

		redis.clear();
		int ret = redis.zadd(key, members);
		if (ret < 0)
		{
			printf("add key: %s error\r\n", key.c_str());
			return false;
		}
		else if (i < 10)
			printf("add ok, key: %s, ret: %d\r\n",
				key.c_str(), ret);
		members.clear();
	}

	return true;
}

static bool test_zcard(acl::redis_zset& redis, int n)
{
	acl::string key;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		int ret = redis.zcard(key.c_str());
		if (ret < 0)
		{
			printf("zcard key: %s error\r\n", key.c_str());
			return false;
		}
		else if (i < 10)
			printf("zcard ok, key: %s, count: %d\r\n",
				key.c_str(), ret);
	}

	return true;
}

static bool test_zcount(acl::redis_zset& redis, int n)
{
	acl::string key;
	double min = 2, max = 100;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		int ret = redis.zcount(key.c_str(), min, max);
		if (ret < 0)
		{
			printf("zcount key: %s error\r\n", key.c_str());
			return false;
		}
		else if (i < 10)
			printf("zcount ok, key: %s, count: %d\r\n",
				key.c_str(), ret);
	}

	return true;
}

static bool test_zincrby(acl::redis_zset& redis, int n)
{
	acl::string key;
	double inc = 2.5, result;
	acl::string member;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);

		for (int j = 0; j < 1000; j++)
		{
			member.format("member_%d", j);
			redis.clear();
			if (redis.zincrby(key.c_str(), inc, member.c_str(),
				&result) == false)
			{
				printf("zincrby error, key: %s\r\n", key.c_str());
				return false;
			}
			else if (j < 10 && i * j < 100)
				printf("zincrby ok key: %s, result: %.2f\r\n",
					key.c_str(), result);
		}
	}

	return true;
}

static bool test_zrange(acl::redis_zset& redis, int n)
{
	acl::string key;
	std::vector<acl::string> result;
	int start = 0, stop = 10;

	printf("===============test zrange=============================\r\n");
	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		result.clear();

		int ret = redis.zrange(key.c_str(), start, stop, &result);
		if (ret < 0)
		{
			printf("zrange error, key: %s\r\n", key.c_str());
			return false;
		}
		else if (i >= 10)
		{
			result.clear();
			continue;
		}

		printf("zrange ok, key: %s, ret: %d\r\n", key.c_str(), ret);
		std::vector<acl::string>::const_iterator cit;
		for (cit = result.begin(); cit != result.end(); ++cit)
		{
			if (cit != result.begin())
				printf(", ");
			printf("%s", (*cit).c_str());
		}
		printf("\r\n");
		result.clear();
	}

	printf("===============test zrange_with_scores=================\r\n");
	std::vector<std::pair<acl::string, double> > result2;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		result.clear();

		int ret = redis.zrange_with_scores(key.c_str(), start, stop,
				result2);
		if (ret < 0)
		{
			printf("zrange error, key: %s\r\n", key.c_str());
			break;
		}
		else if (i >= 10)
		{
			result2.clear();
			continue;
		}

		printf("zrange ok, key: %s, ret: %d\r\n", key.c_str(), ret);

		std::vector<std::pair<acl::string, double> >::const_iterator cit;
		for (cit = result2.begin(); cit != result2.end(); ++cit)
		{
			if (cit != result2.begin())
				printf(", ");
			printf("%s: %.2f", cit->first.c_str(), cit->second);
		}
		printf("\r\n");
		result2.clear();
	}

	return true;
}

static bool test_zrangebyscore(acl::redis_zset& redis, int n)
{
	acl::string key;
	double min = 2, max = 10;

	printf("================test zrangebyscore=====================\r\n");
	std::vector<acl::string> result;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		result.clear();

		int ret = redis.zrangebyscore(key.c_str(), min, max, &result);
		if (ret < 0)
		{
			printf("zrangebyscore error, key: %s\r\n", key.c_str());
			return false;
		}
		else if (i >= 10)
		{
			result.clear();
			continue;
		}

		printf("zrangebyscore ok, key: %s, ret: %d\r\n",
			key.c_str(), ret);
		std::vector<acl::string>::const_iterator cit;
		for (cit = result.begin(); cit != result.end(); ++cit)
		{
			if (cit != result.begin())
				printf(", ");
			printf("%s", (*cit).c_str());
		}
		printf("\r\n");
		result.clear();
	}

	printf("===========test zrangebyscore_with_scores==============\r\n");
	std::vector<std::pair<acl::string, double> > result2;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		result.clear();

		int ret = redis.zrangebyscore_with_scores(key.c_str(),
			min, max, result2);
		if (ret < 0)
		{
			printf("zrangebyscore_with_scores error, key: %s\r\n",
				key.c_str());
			return false;
		}
		else if (i >= 10)
		{
			result2.clear();
			continue;
		}

		printf("zrangebyscore_with_scores ok, key: %s, ret: %d\r\n",
			key.c_str(), ret);
		std::vector<std::pair<acl::string, double> >::const_iterator cit;
		for (cit = result2.begin(); cit != result2.end(); ++cit)
		{
			if (cit != result2.begin())
				printf(", ");
			printf("%s: %.2f", cit->first.c_str(),
				cit->second);
		}
		printf("\r\n");
		result2.clear();
	}

	return true;
}

static bool test_zrank(acl::redis_zset& redis, int n)
{
	acl::string key;
	acl::string member;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);

		for (int j = 0; j < 1000; j++)
		{
			member.format("member_%d", j);
			redis.clear();
			int ret = redis.zrank(key.c_str(), member.c_str());
			if (ret < 0)
			{
				printf("zrank error, key: %s\r\n", key.c_str());
				return false;
			}
			else if (j > 0 && j * i < 100)
				printf("zrank ok, key: %s, member:%s, "
					"rank: %d\r\n", key.c_str(),
					member.c_str(), ret);
		}
	}

	return true;
}

static bool test_zrem(acl::redis_zset& redis, int n)
{
	acl::string key;
	acl::string member;
	std::vector<acl::string> members;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);

		for (int j = 900; j < 1000; j++)
		{
			member.format("member_%d", j);
			members.push_back(member);
		}

		redis.clear();
		int ret = redis.zrem(key.c_str(), members);
		if (ret < 0)
		{
			printf("zrem error, key: %s\r\n", key.c_str());
			return false;
		}
		else if (i < 10)
			printf("zrem ok, key: %s, ret: %d\r\n",
				key.c_str(), ret);
	}

	return true;
}

static bool test_zscore(acl::redis_zset& redis, int n)
{
	acl::string key;
	acl::string member;
	bool ret;
	double result;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);

		for (int j = 0; j < 1000; j++)
		{
			member.format("member_%d", j);
			redis.clear();
			ret = redis.zscore(key.c_str(), member.c_str(),
				result);
			if (ret == false)
			{
				printf("zscore error, key: %s\r\n",
					key.c_str());
				return false;
			}
			else if (j > 0 && j * i < 100)
				printf("zscore ok, key: %s, member:%s, "
					"score: %.2f\r\n", key.c_str(),
					member.c_str(), result);
		}
	}

	return true;
}

static bool test_zunionstore(acl::redis_zset& redis, int n)
{
	acl::string dest_key, src1_key, src2_key;
	std::vector<acl::string> src_keys;

	for (int i = 0; i < n; i++)
	{
		dest_key.format("zset_dest_key_%d", i);
		src1_key.format("zset_src1_key_%d", i);
		src_keys.push_back(src1_key);
		src2_key.format("zset_src2_key_%d", i);
		src_keys.push_back(src2_key);

		redis.clear();
		int ret = redis.zunionstore(dest_key.c_str(), src_keys);
		if (ret < 0)
		{
			printf("zunionstore error, dest: %s\r\n",
				dest_key.c_str());
			return false;
		}
		src_keys.clear();
	}

	return true;
}

static bool test_zinterstore(acl::redis_zset& redis ,int n)
{
	acl::string dest_key, src1_key, src2_key;
	std::vector<acl::string> src_keys;

	for (int i = 0; i < n; i++)
	{
		dest_key.format("zset_dest_key_%d", i);
		src1_key.format("zset_src1_key_%d", i);
		src_keys.push_back(src1_key);
		src2_key.format("zset_src2_key_%d", i);
		src_keys.push_back(src2_key);

		redis.clear();
		int ret = redis.zinterstore(dest_key.c_str(), src_keys);
		if (ret < 0)
		{
			printf("zinterstore error, dest: %s\r\n",
				dest_key.c_str());
			return false;
		}
		src_keys.clear();
	}

	return true;
}

static bool test_zscan(acl::redis_zset& redis, int n)
{
	acl::string key;
	int   ret = 0;
	std::vector<std::pair<acl::string, double> > result;
	std::vector<std::pair<acl::string, double> >::const_iterator cit;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		while (true)
		{
			redis.clear();
			ret = redis.zscan(key.c_str(), ret, result);
			if (ret < 0)
			{
				printf("zscan failed, key: %s\r\n",
					key.c_str());
				return false;
			}
			
			if (i >= 10)
			{
				if (ret == 0)
					break;
			}

			for (cit = result.begin(); cit != result.end(); ++cit)
			{
				printf("%s: %.2f\r\n", cit->first.c_str(),
					cit->second);
			}

			if (ret == 0)
			{
				printf("zscan over, key: %s\r\n", key.c_str());
				break;
			}
		}
	}

	return true;
}

static bool test_zrangebylex(acl::redis_zset& redis, int n)
{
	acl::string key;
	const char* min = "[aaa", *max = "(g";
	std::vector<acl::string> result;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		int ret = redis.zrangebylex(key.c_str(), min, max, &result);
		if (ret < 0)
		{
			printf("zrangebylex error(%s), key: %s\r\n",
				redis.result_error(), key.c_str());
			return false;
		}
		if (i >= 10)
		{
			result.clear();
			continue;
		}

		std::vector<acl::string>::const_iterator cit;
		for (cit = result.begin(); cit != result.end(); ++cit)
		{
			if (cit != result.begin())
				printf(", ");
			printf("%s", (*cit).c_str());
		}
		printf("\r\n");
	}

	return true;
}

static bool test_zlexcount(acl::redis_zset& redis, int n)
{
	acl::string key;
	const char* min = "[aaa", *max = "(g";

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		int ret = redis.zlexcount(key.c_str(), min, max);
		if (ret < 0)
		{
			printf("zlexcount error, key: %s\r\n", key.c_str());
			return false;
		}
		if (i >= 10)
			continue;
		printf("key: %s, count: %d\r\n", key.c_str(), ret);
	}

	return true;
}

static bool test_zremrangebylex(acl::redis_zset& redis, int n)
{
	acl::string key;
	const char* min = "[aaa", *max = "(g";

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		int ret = redis.zremrangebylex(key.c_str(), min, max);
		if (ret < 0)
		{
			printf("zremrangebylex error, key: %s\r\n",
				key.c_str());
			return false;
		}
		if (i >= 10)
			continue;
		printf("key: %s, count: %d\r\n", key.c_str(), ret);
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
		"-a cmd[zadd|zcard|zcount|zincrby|zrange|zrangebyscore|zrank|zrem|zscore|zunionstore|zinterstore|zscan|zrangebylex|zlexcount|zremrangebylex]\r\n",
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

	acl::redis_zset redis;

	if (cluster_mode)
		redis.set_cluster(&cluster);
	else
		redis.set_client(&client);

	bool ret;

	if (cmd == "zadd")
		ret = test_zadd(redis, n);
	else if (cmd == "zcard")
		ret = test_zcard(redis, n);
	else if (cmd == "zcount")
		ret = test_zcount(redis, n);
	else if (cmd == "zincrby")
		ret = test_zincrby(redis, n);
	else if (cmd == "zrange")
		ret = test_zrange(redis, n);
	else if (cmd == "zrangebyscore")
		ret = test_zrangebyscore(redis, n);
	else if (cmd == "zrank")
		ret = test_zrank(redis, n);
	else if (cmd == "zrem")
		ret = test_zrem(redis, n);
	else if (cmd == "zscore")
		ret = test_zscore(redis, n);
	else if (cmd == "zunionstore")
		ret = test_zunionstore(redis, n);
	else if (cmd == "zinterstore")
		ret = test_zinterstore(redis, n);
	else if (cmd == "zscan")
		ret = test_zscan(redis, n);
	else if (cmd == "zrangebylex")
		ret = test_zrangebylex(redis, n);
	else if (cmd == "zlexcount")
		ret = test_zlexcount(redis, n);
	else if (cmd == "zremrangebylex")
		ret = test_zremrangebylex(redis, n);
	else if (cmd == "all")
	{
		ret = test_zadd(redis, n)
			&& test_zcard(redis, n)
			&& test_zcount(redis, n)
			&& test_zincrby(redis, n)
			&& test_zrange(redis, n)
			&& test_zrangebyscore(redis, n)
			&& test_zrank(redis, n)
			&& test_zrem(redis, n)
			&& test_zscore(redis, n);
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
