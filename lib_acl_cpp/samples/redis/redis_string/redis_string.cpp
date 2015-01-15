#include "stdafx.h"

static acl::string __keypre("test_key");

static void test_set(acl::redis_string& option, int n)
{
	acl::string key;
	acl::string value;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		value.format("value_%s", key.c_str());

		if (option.set(key.c_str(), value.c_str()) == false)
		{
			printf("set key: %s error\r\n", key.c_str());
			break;
		}
		else if (i < 10)
			printf("set key: %s ok\r\n", key.c_str());
	}
}

static void test_setex(acl::redis_string& option, int n, int ttl)
{
	acl::string key, value;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		value.format("value_%s", key.c_str());
		if (option.setex(key.c_str(), value.c_str(), ttl) == false)
		{
			printf("setex key: %s error\r\n", key.c_str());
			break;
		}
		else if (i < 10)
			printf("setex key: %s, ttl: %d\r\n", key.c_str(), ttl);
	}
}

static void test_setnx(acl::redis_string& option, int n)
{
	acl::string key;
	acl::string value;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		value.format("_setnx_%s", key.c_str());

		int ret = option.setnx(key.c_str(), value.c_str());
		if (ret < 0)
		{
			printf("setnx key: %s error\r\n", key.c_str());
			break;
		}
		printf("%s: ret: %d, key: %s\r\n", __FUNCTION__, ret,
			key.c_str());
	}
}

static void test_append(acl::redis_string& option, int n)
{
	acl::string key;
	acl::string value;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		value.format("_append_%d", i);

		if (option.append(key.c_str(), value.c_str()) < 0)
		{
			printf("append key: %s\r\n", key.c_str());
			break;
		}
	}
}

static void test_get(acl::redis_string& option, int n)
{
	acl::string key;
	acl::string value;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		//key.format("key1_%s_%d", __keypre.c_str(), i);
		value.clear();

		if (option.get(key.c_str(), value) == false)
		{
			printf("get key: %s\r\n", key.c_str());
			break;
		}
		else if (i < 10)
			printf("key: %s, value: %s, len: %d\r\n",
				key.c_str(), value.c_str(),
				(int) value.length());
	}
}

static void test_getset(acl::redis_string& option, int n)
{
	acl::string key;
	acl::string value;
	acl::string result;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		value.format("getset_%s", key.c_str());
		result.clear();
		if (option.getset(key.c_str(), value.c_str(), result) == false)
		{
			printf("getset error, key: %s\r\n", key.c_str());
			break;
		}
		else if (i < 10)
			printf("getset: key: %s, old value: %s\r\n",
				key.c_str(), result.c_str());
	}
}

static void test_strlen(acl::redis_string& option, int n)
{
	acl::string key;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		int ret = option.str_len(key.c_str());
		if (ret < 0)
		{
			printf("str_len error, key: %s\r\n", key.c_str());
			break;
		}
		else if (i < 10)
			printf("key: %s's value's length: %d\r\n",
				key.c_str(), ret);
	}
}

static void test_mset(acl::redis_string& option, int n)
{
	acl::string key1, key2, key3;
	acl::string val1, val2, val3;
	std::map<acl::string, acl::string> objs;

	for (int i = 0; i < n; i++)
	{
		key1.format("key1_%s_%d", __keypre.c_str(), i);
		key2.format("key2_%s_%d", __keypre.c_str(), i);
		key3.format("key3_%s_%d", __keypre.c_str(), i);

		val1.format("val1_%s", key1.c_str());
		val2.format("val2_%s", key2.c_str());
		val3.format("val3_%s", key3.c_str());

		objs[key1] = val1;
		objs[key2] = val2;
		objs[key3] = val3;

		if (option.mset(objs) == false)
		{
			printf("mset error\r\n");
			break;
		}
		else if (i < 10)
		{
			printf("mset ok, %s=%s, %s=%s, %s=%s\r\n",
				key1.c_str(), val1.c_str(),
				key2.c_str(), val2.c_str(),
				key3.c_str(), val3.c_str());
		}
		objs.clear();
	}
}

static void test_mget(acl::redis_string& option, int n)
{
	acl::string key1, key2, key3;
	std::vector<acl::string> result;
	const char* keys[3];

	for (int i = 0; i < n; i++)
	{
		key1.format("key1_%s_%d", __keypre.c_str(), i);
		key2.format("key2_%s_%d", __keypre.c_str(), i);
		key3.format("key3_%s_%d", __keypre.c_str(), i);
		keys[0] = key1.c_str();
		keys[1] = key2.c_str();
		keys[2] = key3.c_str();

		result.clear();
		if (option.mget(keys, 3, &result) == false)
		{
			printf("mset error\r\n");
			break;
		}
		else if (i >= 10)
			continue;

		size_t size = option.mget_size();
		printf("size: %lu\r\n", (unsigned long) size);

		size_t j;
		for (j = 0; j < size; j++)
		{
			const char* val = option.mget_value(j);
			printf("mget ok, %s=%s\r\n",
				keys[j], val ? val : "null");
		}

		std::vector<acl::string>::const_iterator it= result.begin();
		for (j = 0; it != result.end(); ++it, j++)
			printf("mget %s=%s\r\n", keys[j], (*it).c_str());
	}
}

static void test_msetnx(acl::redis_string& option, int n)
{
	acl::string key1, key2, key3;
	acl::string val1, val2, val3;
	std::map<acl::string, acl::string> objs;
	int   ret;

	for (int i = 0; i < n; i++)
	{
		key1.format("key1_%s_%d", __keypre.c_str(), i);
		key2.format("key2_%s_%d", __keypre.c_str(), i);
		key3.format("key3_%s_%d", __keypre.c_str(), i);

		val1.format("val1_%s", key1.c_str());
		val2.format("val2_%s", key2.c_str());
		val3.format("val3_%s", key3.c_str());

		objs[key1] = val1;
		objs[key2] = val2;
		objs[key3] = val3;

		ret = option.msetnx(objs);
		if (ret < 0)
		{
			printf("mset error\r\n");
			break;
		}
		else if (i < 10)
		{
			printf("msetnx ret: %d, %s=%s, %s=%s, %s=%s\r\n", ret,
				key1.c_str(), val1.c_str(),
				key2.c_str(), val2.c_str(),
				key3.c_str(), val3.c_str());
		}
		objs.clear();
	}
}

static void test_setrange(acl::redis_string& option, int n)
{
	acl::string key, value;
	unsigned int off = 5;
	int   ret;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		value.format("range_value_%s", key.c_str());
		ret = option.setrange(key.c_str(), off, value.c_str());
		if (ret < 0)
		{
			printf("setrange error, key: %s, off: %u, value: %s\r\n",
				key.c_str(), off, value.c_str());
			break;
		}
		else if (i < 10)
			printf("setrange ok, key: %s, off: %u, value: %s\r\n",
				key.c_str(), off, value.c_str());
	}
}

static void test_getrange(acl::redis_string& option, int n)
{
	acl::string key, value;
	int start = 5, end = 10;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		value.clear();
		if (option.getrange(key, start, end, value) == false)
		{
			printf("getrange error, key: %s, start: %d, end: %d\r\n",
				key.c_str(), start, end);
			break;
		}
		else if (i >= 10)
			continue;

		printf("getrange ok, key: %s, start: %d, end: %d, value: %s\r\n",
			key.c_str(), start, end, value.c_str());
	}
}

static void test_setbit(acl::redis_string& option, int n)
{
	acl::string key;
	unsigned off = 5;

	for (int i = 0; i < n; i++)
	{
		key.format("bit_%s_%d", __keypre.c_str(), i);
		if (option.setbit(key.c_str(), off, 1) == false)
		{
			printf("setbit error, key: %s, off: %u\r\n",
				key.c_str(), off);
			break;
		}
		else if (i >= 10)
			continue;

		printf("setbit ok, key: %s, off: %d\r\n", key.c_str(), off);
	}
}

static void test_getbit(acl::redis_string& option, int n)
{
	acl::string key;
	unsigned off = 5;
	int bit;

	for (int i = 0; i < n; i++)
	{
		key.format("bit_%s_%d", __keypre.c_str(), i);
		if (option.getbit(key.c_str(), off, bit) == false)
		{
			printf("getbit error, key: %s, off: %u\r\n",
				key.c_str(), off);
			break;
		}
		else if (i >= 10)
			continue;

		printf("getbit ok, key: %s, off: %d, bit: %d\r\n",
			key.c_str(), off, bit);
	}
}

static void test_bitcount(acl::redis_string& option, int n)
{
	acl::string key;
	int   ret;

	for (int i = 0; i < n; i++)
	{
		key.format("bit_%s_%d", __keypre.c_str(), i);
		ret = option.bitcount(key.c_str());
		if (ret < 0)
		{
			printf("bitcount error, key: %s\r\n", key.c_str());
			break;
		}
		else if (i < 10)
			printf("bitcount ok, key: %s, ret: %d\r\n",
				key.c_str(), ret);
	}
}

static void test_bitop_and(acl::redis_string& option, int n)
{
	const char* keys[3];
	acl::string key, key1, key2, key3;
	int   ret;

	for (int i = 0; i < n; i++)
	{
		key.format("bit_%s_%d", __keypre.c_str(), i);
		key1.format("bit_%s_%d", __keypre.c_str(), i % 1);
		key2.format("bit_%s_%d", __keypre.c_str(), i % 2);
		key3.format("bit_%s_%d", __keypre.c_str(), i % 3);
		keys[0] = key1.c_str();
		keys[1] = key2.c_str();
		keys[2] = key3.c_str();

		ret = option.bitop_and(key.c_str(), keys, 3);
		if (ret < 0)
		{
			printf("bitop_and error, key: %s\r\n", key.c_str());
			break;
		}
		else if (i < 10)
			printf("bitop_and ok, key: %s, bits: %u\n",
				key.c_str(), ret);
	}
}

static void test_bitop_or(acl::redis_string& option, int n)
{
	const char* keys[3];
	acl::string key, key1, key2, key3;
	int   ret;

	for (int i = 0; i < n; i++)
	{
		key.format("bit_%s_%d", __keypre.c_str(), i);
		key1.format("bit_%s_%d", __keypre.c_str(), i % 1);
		key2.format("bit_%s_%d", __keypre.c_str(), i % 2);
		key3.format("bit_%s_%d", __keypre.c_str(), i % 3);
		keys[0] = key1.c_str();
		keys[1] = key2.c_str();
		keys[2] = key3.c_str();

		ret = option.bitop_or(key.c_str(), keys, 3);
		if (ret < 0)
		{
			printf("bitop_or error, key: %s\r\n", key.c_str());
			break;
		}
		else if (i < 10)
			printf("bitop_or ok, key: %s, bits: %u\n",
			key.c_str(), ret);
	}
}

static void test_bitop_xor(acl::redis_string& option, int n)
{
	const char* keys[3];
	acl::string key, key1, key2, key3;
	int   ret;

	for (int i = 0; i < n; i++)
	{
		key.format("bit_%s_%d", __keypre.c_str(), i);
		key1.format("bit_%s_%d", __keypre.c_str(), i % 1);
		key2.format("bit_%s_%d", __keypre.c_str(), i % 2);
		key3.format("bit_%s_%d", __keypre.c_str(), i % 3);
		keys[0] = key1.c_str();
		keys[1] = key2.c_str();
		keys[2] = key3.c_str();

		ret = option.bitop_xor(key.c_str(), keys, 3);
		if (ret < 0)
		{
			printf("bitop_xor error, key: %s\r\n", key.c_str());
			break;
		}
		else if (i < 10)
			printf("bitop_xor ok, key: %s, bits: %u\n",
			key.c_str(), ret);
	}
}

static void test_incr(acl::redis_string& option, int n)
{
	acl::string key;
	long long int result;

	for (int i = 0; i < n; i++)
	{
		key.format("incr_%s_%d", __keypre.c_str(), i);
		if (option.incr(key.c_str(), &result) == false)
		{
			printf("incr error, key: %s\r\n", key.c_str());
			break;
		}
		else if (i < 10)
			printf("incr ok, key: %s, result: %lld\r\n",
				key.c_str(), result);
	}
}

static void test_incrby(acl::redis_string& option, int n)
{
	acl::string key;
	long long int result;

	for (int i = 0; i < n; i++)
	{
		key.format("incr_%s_%d", __keypre.c_str(), i);
		if (option.incrby(key.c_str(), 10, &result) == false)
		{
			printf("incrby error, key: %s\r\n", key.c_str());
			break;
		}
		else if (i < 10)
			printf("incrby ok, key: %s, result: %lld\r\n",
				key.c_str(), result);
	}
}

static void test_incrbyfloat(acl::redis_string& option, int n)
{
	acl::string key;
	double result;

	for (int i = 0; i < n; i++)
	{
		key.format("incrbyfloat_%s_%d", __keypre.c_str(), i);
		if (option.incrbyfloat(key.c_str(), 8.8, &result) == false)
		{
			printf("incrbyfloat error, key: %s\r\n", key.c_str());
			break;
		}
		else if (i < 10)
			printf("incrbyfloat ok, key: %s, result: %.2f\r\n",
				key.c_str(), result);
	}
}

static void test_decr(acl::redis_string& option, int n)
{
	acl::string key;
	long long int result;

	for (int i = 0; i < n; i++)
	{
		key.format("incr_%s_%d", __keypre.c_str(), i);
		if (option.decr(key.c_str(), &result) == false)
		{
			printf("decr error, key: %s\r\n", key.c_str());
			break;
		}
		else if (i < 10)
			printf("decr ok, key: %s, result: %lld\r\n",
				key.c_str(), result);
	}
}

static void test_decrby(acl::redis_string& option, int n)
{
	acl::string key;
	long long int result;

	for (int i = 0; i < n; i++)
	{
		key.format("incr_%s_%d", __keypre.c_str(), i);
		if (option.decrby(key.c_str(), 10, &result) == false)
		{
			printf("decrby error, key: %s\r\n", key.c_str());
			break;
		}
		else if (i < 10)
			printf("decrby ok, key: %s, result: %lld\r\n",
				key.c_str(), result);
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s redis_addr[127.0.0.1:6380]\r\n"
		"-n count\r\n"
		"-C connect_timeout[default: 10]\r\n"
		"-T rw_timeout[default: 10]\r\n"
		"-t object timeout[default: 10]\r\n"
		"-a cmd\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 1, conn_timeout = 10, rw_timeout = 10, ttl = 10;
	acl::string addr("127.0.0.1:6380"), cmd;

	while ((ch = getopt(argc, argv, "hs:n:C:T:a:t:")) > 0)
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
		case 't':
			ttl = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);
	acl::redis_client client(addr.c_str(), conn_timeout, rw_timeout);
	acl::redis_string option(client);

	if (cmd == "set")
		test_set(option, n);
	else if (cmd == "setex")
		test_setex(option, n, ttl);
	else if (cmd == "setnx")
		test_setnx(option, n);
	else if (cmd == "append")
		test_append(option, n);
	else if (cmd == "get")
		test_get(option, n);
	else if (cmd == "getset")
		test_getset(option, n);
	else if (cmd == "strlen")
		test_strlen(option, n);
	else if (cmd == "mset")
		test_mset(option, n);
	else if (cmd == "mget")
		test_mget(option, n);
	else if (cmd == "msetnx")
		test_msetnx(option, n);
	else if (cmd == "setrange")
		test_setrange(option, n);
	else if (cmd == "getrange")
		test_getrange(option, n);
	else if (cmd == "setbit")
		test_setbit(option, n);
	else if (cmd == "getbit")
		test_getbit(option, n);
	else if (cmd == "bitcount")
		test_bitcount(option, n);
	else if (cmd == "bitop_and")
		test_bitop_and(option, n);
	else if (cmd == "bitop_or")
		test_bitop_or(option, n);
	else if (cmd == "bitop_xor")
		test_bitop_xor(option, n);
	else if (cmd == "incr")
		test_incr(option, n);
	else if (cmd == "incrby")
		test_incrby(option, n);
	else if (cmd == "incrbyfloat")
		test_incrbyfloat(option, n);
	else if (cmd == "decr")
		test_decr(option, n);
	else if (cmd == "decrby")
		test_decrby(option, n);
	else if (cmd == "all")
	{
		test_set(option, n);
		test_setex(option, n, ttl);
		test_setnx(option, n);
		test_append(option, n);
		test_get(option, n);
		test_getset(option, n);
		test_strlen(option, n);
		test_mset(option, n);
		test_mget(option, n);
		test_msetnx(option, n);
		test_setrange(option, n);
		test_getrange(option, n);
		test_setbit(option, n);
		test_getbit(option, n);
		test_bitcount(option, n);
		test_bitop_and(option, n);
		test_bitop_or(option, n);
		test_bitop_xor(option, n);
		test_incr(option, n);
		test_incrby(option, n);
		test_incrbyfloat(option, n);
		test_decr(option, n);
		test_decrby(option, n);
	}
	else
		printf("unknown cmd: %s\r\n", cmd.c_str());

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
