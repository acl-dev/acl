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
		//option.get_client().reset();
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
		// option.get_client().reset();
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
		// option.get_client().reset();
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
		// option.get_client().reset();
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
		// option.get_client().reset();
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
		// option.get_client().reset();
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
		// option.get_client().reset();
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
		// option.get_client().reset();
		objs.clear();
	}
}

static void test_mget(acl::redis_string& option, int n)
{
	acl::string key1, key2, key3;
	std::vector<acl::string> result;

	for (int i = 0; i < n; i++)
	{
		key1.format("key1_%s_%d", __keypre.c_str(), i);
		key2.format("key2_%s_%d", __keypre.c_str(), i);
		key3.format("key3_%s_%d", __keypre.c_str(), i);

		result.clear();
		if (option.mget(&result, key1.c_str(), key2.c_str(),
			key3.c_str(), NULL) == false)
		{
			printf("mset error\r\n");
			break;
		}
		else if (i < 10)
		{
			size_t size = option.mget_size();
			printf("size: %lu\r\n", (unsigned long) size);
			printf("key1: %s\r\n", key1.c_str());
			printf("key2: %s\r\n", key2.c_str());
			printf("key3: %s\r\n", key3.c_str());

			for (size_t j = 0; j < size; j++)
			{
				const char* val = option.mget_value(j);
				printf("mget ok, %s\r\n", val ? val : "null");
			}

			std::vector<acl::string>::const_iterator it;
			for (it = result.begin(); it != result.end(); ++it)
				printf("mget %s\r\n", (*it).c_str());
		}
		// option.get_client().reset();
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
	/*
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
	*/
	else
		printf("unknown cmd: %s\r\n", cmd.c_str());

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
