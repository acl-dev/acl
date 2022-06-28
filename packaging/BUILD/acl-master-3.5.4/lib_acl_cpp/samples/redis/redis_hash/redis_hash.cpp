#include "stdafx.h"

static acl::string __keypre("hash_test_key");

static bool test_hmset(acl::redis_hash& redis, int n)
{
	acl::string key, attr1, attr2, attr3;
	acl::string val1, val2, val3;
	std::map<acl::string, acl::string> attrs;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		attr1.format("attr1");
		attr2.format("attr2");
		attr3.format("attr3");

		val1.format("val1_%s", attr1.c_str());
		val2.format("val2_%s", attr2.c_str());
		val3.format("val3_%s", attr3.c_str());

		attrs[attr1] = val1;
		attrs[attr2] = val2;
		attrs[attr3] = val3;

		redis.clear();
		if (redis.hmset(key.c_str(), attrs) == false)
		{
			printf("hmset error: %s, key: %s\r\n",
				redis.result_error(), key.c_str());
			return false;
		}
		else if (i < 10)
		{
			printf("hmset ok, key: %s, %s=%s, %s=%s, %s=%s\r\n",
				key.c_str(), attr1.c_str(), val1.c_str(),
				attr2.c_str(), val2.c_str(),
				attr3.c_str(), val3.c_str());
		}
		attrs.clear();
	}

	return true;
}

static bool test_hmget(acl::redis_hash& redis, int n)
{
	acl::string key, attr1, attr2, attr3;
	const char* attrs[3];
	std::vector<acl::string> result;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		attr1.format("attr1");
		attr2.format("attr2");
		attr3.format("attr3");
		attrs[0] = attr1.c_str();
		attrs[1] = attr2.c_str();
		attrs[2] = attr3.c_str();

		result.clear();
		redis.clear();
		if (redis.hmget(key, attrs, 3, &result) == false)
		{
			printf("hmget error: %s\r\n", redis.result_error());
			return false;
		}
		else if (i >= 10)
			continue;

		size_t size = redis.result_size();
		printf("size: %lu, key: %s\r\n", (unsigned long) size,
			key.c_str());

		size_t j;
		for (j = 0; j < size; j++)
		{
			const char* val = redis.result_value(j);
			printf("hmget ok, %s=%s\r\n",
				attrs[j], val ? val : "null");
		}

		std::vector<acl::string>::const_iterator it= result.begin();
		for (j = 0; it != result.end(); ++it, j++)
			printf("hmget %s=%s\r\n", attrs[j], (*it).c_str());
	}

	return true;
}

static bool test_hset(acl::redis_hash& redis, int n)
{
	acl::string key;
	acl::string attr, value;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		attr.format("attr1");
		value.format("value_%s", key.c_str());

		redis.clear();
		if (redis.hset(key.c_str(), attr.c_str(),
			value.c_str()) < 0)
		{
			printf("hset key: %s error: %s\r\n",
				key.c_str(), redis.result_error());
			return false;
		}
		else if (i < 10)
			printf("hset key: %s ok\r\n", key.c_str());
	}

	return true;
}

static bool test_hsetnx(acl::redis_hash& redis, int n)
{
	acl::string key;
	acl::string attr, value;
	int ret;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		attr.format("attr4");
		value.format("value_%s", key.c_str());

		redis.clear();
		if ((ret = redis.hsetnx(key.c_str(), attr.c_str(),
			value.c_str())) <0)
		{
			printf("hsetnx key: %s error: %s\r\n",
				key.c_str(), redis.result_error());
			return false;
		}
		else if (i < 10)
			printf("hsetnx key: %s ok, ret: %d\r\n",
				key.c_str(), ret);
	}

	return true;
}

static bool test_hget(acl::redis_hash& redis, int n)
{
	acl::string key;
	acl::string attr, value;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		attr.format("attr1");
		value.clear();

		redis.clear();
		if (redis.hget(key.c_str(), attr.c_str(), value) == false)
		{
			printf("hget key: %s, attr: %s\r\n", key.c_str(),
				attr.c_str());
			return false;
		}
		else if (i >= 10)
			continue;

		printf("key: %s, attr: %s, value: %s, len: %d\r\n",
			key.c_str(), attr.c_str(), value.c_str(),
			(int) value.length());
	}

	return true;
}

static bool test_hgetall(acl::redis_hash& redis, int n)
{
	acl::string key;
	std::map<acl::string, acl::string> result;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		result.clear();

		redis.clear();
		if (redis.hgetall(key.c_str(), result) == false)
		{
			printf("hgetall key: %s\r\n", key.c_str());
			return false;
		}
		else if (i >= 10)
			continue;

		std::map<acl::string, acl::string>::const_iterator cit;
		printf("key: %s\r\n", key.c_str());
		for (cit = result.begin(); cit != result.end(); ++cit)
		{
			printf("attr: %s=%s\r\n", cit->first.c_str(),
				cit->second.c_str());
		}
	}

	return true;
}

static bool test_hdel(acl::redis_hash& redis, int n)
{
	acl::string key, attr;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		attr.format("attr1");

		redis.clear();
		int ret = redis.hdel(key.c_str(), attr.c_str());
		if (ret < 0)
		{
			printf("hdel key: %s error: %s\r\n",
				key.c_str(), redis.result_error());
			return false;
		}
		else if (i < 10)
			printf("hdel ok, key: %s\r\n", key.c_str());
	}

	return true;
}

static bool test_hincrby(acl::redis_hash& redis, int n)
{
	acl::string key, attr;
	long long int result;

	for (int i = 0; i < n; i++)
	{
		key.format("hincr_%s_%d", __keypre.c_str(), i);
		attr.format("attr1");

		redis.clear();
		if (redis.hincrby(key.c_str(), attr.c_str(), 10,
			&result) == false)
		{
			printf("hincrby error: %s, key: %s, attr: %s\r\n",
				redis.result_error(), key.c_str(), attr.c_str());
			return false;
		}
		else if (i < 10)
			printf("hincrby, key: %s, attr: %s, result: %lld\r\n",
				key.c_str(), attr.c_str(), result);
	}

	return true;
}

static bool test_hincrbyfloat(acl::redis_hash& redis, int n)
{
	acl::string key, attr;
	double result;

	for (int i = 0; i < n; i++)
	{
		key.format("hincrbyfloat_%s_%d", __keypre.c_str(), i);
		attr.format("attr1");

		redis.clear();
		if (redis.hincrbyfloat(key.c_str(), attr.c_str(),
			8.8, &result) == false)
		{
			printf("hincrbyfloat error: %s, key: %s\r\n",
				redis.result_error(), key.c_str());
			return false;
		}
		else if (i >= 10)
			continue;

		printf("hincrbyfloat ok, key: %s, attr: %s, result: %.2f\r\n",
			key.c_str(), attr.c_str(), result);
	}

	return true;
}

static bool test_hkeys(acl::redis_hash& redis, int n)
{
	acl::string key;
	std::vector<acl::string> attrs;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		attrs.clear();

		redis.clear();
		if (redis.hkeys(key.c_str(), attrs) == false)
		{
			printf("hkeys error: %s, key: %s\r\n",
				redis.result_error(), key.c_str());
			return false;
		}
		else if (i >= 10)
			continue;

		printf("hkeys ok, key: %s\r\n", key.c_str());
		std::vector<acl::string>::const_iterator cit;
		for (cit = attrs.begin(); cit != attrs.end(); ++cit)
		{
			if (cit != attrs.begin())
				printf(", ");
			printf("%s", (*cit).c_str());
		}
		printf("\r\n");
	}

	return true;
}

static bool test_hexists(acl::redis_hash& redis, int n)
{
	acl::string key, attr;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		attr.format("attr1");

		redis.clear();
		if (redis.hexists(key.c_str(), attr.c_str()) == false)
			printf("no hexists key: %s\r\n", key.c_str());
		else
			printf("hexists key: %s, attr: %s\r\n",
				key.c_str(), attr.c_str());
	}

	return true;
}

static bool test_hlen(acl::redis_hash& redis, int n)
{
	acl::string key;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);

		redis.clear();
		int ret = redis.hlen(key.c_str());
		if (ret < 0)
		{
			printf("hlen error: %s, key: %s\r\n",
				redis.result_error(), key.c_str());
			return false;
		}
		else if (i < 10)
			printf("hlen: %s's value's length: %d\r\n",
				key.c_str(), ret);
	}

	return true;
}

static bool test_hvals(acl::redis_hash& redis, int n)
{
	acl::string key;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		std::vector<acl::string> values;
		bool ret = redis.hvals(key, values);
		if (ret == false) {
			printf("hvals error: %s, key: %s\r\n",
				redis.result_error(), key.c_str());
			return false;
		}
		else if (i < 10) {
			printf("key: %s\r\n", key.c_str());
			for (std::vector<acl::string>::const_iterator cit
				= values.begin(); cit != values.end(); ++cit) {
				printf("  %s\r\n", (*cit).c_str());
			}
			printf("\r\n");
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
		"-I rw_timeout[default: 10]\r\n"
		"-S [if slice request, default: no]\r\n"
		"-c [cluster mode]\r\n"
		"-a cmd[hmset|hmget|hset|hsetnx|hget|hgetall|hincrby|hincrbyfloat|hkeys|hexists|hlen|hdel|hvals]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 1, conn_timeout = 10, rw_timeout = 10;
	acl::string addr("127.0.0.1:6379"), cmd;
	bool slice_req = false, cluster_mode = false;

	while ((ch = getopt(argc, argv, "hs:n:C:I:a:Sc")) > 0)
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
		case 'I':
			rw_timeout = atoi(optarg);
			break;
		case 'a':
			cmd = optarg;
			break;
		case 'S':
			slice_req = true;
			break;
		case 'c':
			cluster_mode = true;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl::redis_client_cluster cluster;
	cluster.set(addr.c_str(), 100, conn_timeout, rw_timeout);

	acl::redis_client client(addr.c_str(), conn_timeout, rw_timeout);
	client.set_slice_request(slice_req);

	acl::redis_hash redis;
	if (cluster_mode)
		redis.set_cluster(&cluster);
	else
		redis.set_client(&client);

	bool ret;

	if (cmd == "hmset")
		ret = test_hmset(redis, n);
	else if (cmd == "hmget")
		ret = test_hmget(redis, n);
	else if (cmd == "hset")
		ret = test_hset(redis, n);
	else if (cmd == "hsetnx")
		ret = test_hsetnx(redis, n);
	else if (cmd == "hget")
		ret = test_hget(redis, n);
	else if (cmd == "hgetall")
		ret = test_hgetall(redis, n);
	else if (cmd == "hdel")
		ret = test_hdel(redis, n);
	else if (cmd == "hincrby")
		ret = test_hincrby(redis, n);
	else if (cmd == "hincrbyfloat")
		ret = test_hincrbyfloat(redis, n);
	else if (cmd == "hkeys")
		ret = test_hkeys(redis, n);
	else if (cmd == "hexists")
		ret = test_hexists(redis, n);
	else if (cmd == "hlen")
		ret = test_hlen(redis, n);
	else if (cmd == "hvals")
		ret = test_hvals(redis, n);
	else if (cmd == "all")
	{
		ret = test_hmset(redis, n)
			&& test_hmget(redis, n)
			&& test_hset(redis, n)
			&& test_hsetnx(redis, n)
			&& test_hget(redis, n)
			&& test_hgetall(redis, n)
			&& test_hincrby(redis, n)
			&& test_hincrbyfloat(redis, n)
			&& test_hkeys(redis, n)
			&& test_hexists(redis, n)
			&& test_hlen(redis, n)
			&& test_hdel(redis, n)
			&& test_hvals(redis, n);
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
