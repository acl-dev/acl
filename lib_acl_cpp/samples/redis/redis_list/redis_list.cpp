#include "stdafx.h"

/**
 * author: niukey@qq.com
 * date: 2015.1.27
 * modify by zsx
 */

static const char *__key = "list";
static const int max_print_line = 50;
static const int __value_len = 32;

#define DELETE_VALUES(values,n)\
do\
{  for(int i = 0; i < (n) ; ++i)\
	delete [](values)[i];\
   delete [](values);\
} while (0);


static bool test_lpush(acl::redis_list& redis, int n)
{
	acl::string value;

	for (int i = 0; i < n; i++)
	{
		value.format("%s_%d", __key, i);
		redis.clear();
		int ret = redis.lpush(__key, value.c_str(), NULL);
		if (ret <= 0)
		{
			printf("lpush key: %s error: %s, ret: %d\r\n",
				__key, redis.result_error(), ret);
			return false;
		}
		else if (i < max_print_line)
			printf("lpush ok, key:%s, value:%s, ret: %d\r\n",
				__key, value.c_str(), ret);
	}

	return true;
}

static bool test_lpush2(acl::redis_list& redis, int n)
{
	char** value = new char *[n+1];
	for (int i = 0; i < n; i++)
	{
		value[i] = new char[__value_len];
		acl::safe_snprintf(value[i], __value_len, "%s_%d", __key, i);
	}
	value[n] = NULL;

	redis.clear();
	int ret = redis.lpush(__key, (const char**) value, n);
	if (ret <= 0)
	{
		printf("lpush key: %s error: %s\r\n", __key, redis.result_error());
		return false;
	}
	else
		printf("lpush key:%s ret : %d ok\r\n", __key, ret);

	for (int i = 0; i < n; ++i)
		delete value[i];

	delete [] value;

	return true;
}

static bool test_lpush3(acl::redis_list &redis, int n)
{
	std::vector<acl::string> values;
	acl::string buf;

	for (int i = 0; i < n; ++i)
	{
		buf.format("%s_%d", __key, i);
		values.push_back(buf);
	}

	redis.clear();
	int ret = redis.lpush(__key, values);
	if (ret <= 0)
	{
		printf("lpush key:%s error: %s\r\n", __key, redis.result_error());
		return false;
	}
	else
		printf("lpush key:%s ret : %d ok\r\n", __key, ret);

	return true;
}

static bool test_lpush4(acl::redis_list &redis, int n)
{
	std::vector<const char *> values;
	char **value  = new char *[n];

	for (int i  = 0; i < n; ++i)
	{
		value[i] = new char[__value_len];
		acl::safe_snprintf(value[i], __value_len, "%s_%d", __key, i);
		values.push_back(value[i]);
	}

	redis.clear();
	int ret = redis.lpush(__key, (std::vector<const char*>&) values);
	if(ret <= 0)
	{
		printf("lpush key:%s error: %s\r\n", __key, redis.result_error());
		return false;
	}
	else
		printf("lpush key:%s ret : %d ok\r\n", __key, ret);

	for (int i = 0; i < n; ++i)
		delete value[i];

	delete [] value;

	return true;
}

static bool test_lpush5(acl::redis_list &redis, int n)
{
	std::vector<const char*> values;
	char **value  = new char *[n];
	for (int i  = 0; i < n; ++i)
	{
		value[i] = new char[__value_len];
		acl::safe_snprintf(value[i], __value_len, "%s_%d", __key, i);
		values.push_back(value[i]);
	}

	redis.clear();
	int ret = redis.lpush(__key, values);
	if(ret <= 0)
	{
		printf("lpush key:%s error: %s\r\n", __key, redis.result_error());
		return false;
	}
	else
		printf("lpush key:%s ret : %d ok\r\n", __key, ret);

	for (int i = 0; i < n; ++i)
		delete value[i];

	delete [] value;

	return true;
}

static bool test_lpush6(acl::redis_list &redis, int n)
{
	char **values = new char *[n];
	size_t *lens = new size_t[n];
	for (int i = 0; i < n ;++i)
	{
		values[i] = new char [__value_len];
		acl::safe_snprintf(values[i],__value_len, "%s_%d", __key, i);
		lens[i] = strlen(values[i]);
	}

	redis.clear();
	int ret = redis.lpush(__key, (const char**)values, lens, n);
	if (ret <= 0)
	{
		printf("lpush key:%s error: %s\r\n", __key, redis.result_error());
		return false;
	}
	else
		printf("lpush key:%s ret : %d ok\r\n", __key, ret);

	for (int i = 0; i < n; ++i)
		delete values[i];

	delete [] values;

	return true;
}

static bool test_rpush(acl::redis_list& redis, int n)
{
	acl::string value;
	int ret;
	for (int i = 0; i < n; ++i)
	{
		value.format("%s_%d",__key, i);
		redis.clear();
		ret = redis.rpush(__key, value.c_str(), NULL);
		if (ret <= 0)
		{
			printf("rpush key:%s error, %s\r\n",
				__key, redis.result_error());
			return false;
		}
		else if (i < max_print_line)
		{
			printf("rpush key:%s, value:%s ok \r\n",
				__key, value.c_str());
		}
	}

	return true;
}

static bool test_rpush2(acl::redis_list& redis, int n)
{
	char **values = new char *[n];
	for (int i = 0 ; i < n ; ++ i)
	{
		values[i] = new char[__value_len];
		acl::safe_snprintf(values[i], __value_len, "%s_%d", __key, i);
	}

	redis.clear();
	int ret = redis.rpush(__key, (const char **)values, n);
	if(ret < 0)
		printf("rpush key:%s error: %s\r\n",__key, redis.result_error());
	else
		printf("rpush key:%s ret : %d ok\r\n", __key, ret);

	DELETE_VALUES(values, n);

	return ret > 0 ? true : false;
}

static bool test_rpush3(acl::redis_list &redis, int n)
{
	std::vector<acl::string> values;
	acl::string value;
	for (int i = 0; i < n; ++i)
	{
		value.format("%s_%d", __key, i);
		values.push_back(value);
	}

	redis.clear();
	int ret = redis.rpush(__key, values);
	if (ret <= 0)
	{
		printf("rpush key:%s error: %s", __key, redis.result_error());
		return false;
	}
	else
		printf("rpush key:%s ret : %d ok\r\n", __key, ret);

	return true;
}

static bool test_rpush4(acl::redis_list &redis, int n)
{
	std::vector<const char*> values;
	for (int i = 0; i < n; ++i) 
	{
		char *value = new char[__value_len];
		acl::safe_snprintf(value, __value_len, "%s_%d", __key, i);
		values.push_back(value);
	}

	redis.clear();
	int ret = redis.rpush(__key, (const std::vector<const char*>&) values);
	if(ret <= 0)
	{
		printf("rpush key:%s ok",__key);
		return false;
	}
	else
		printf("rpush key:%s ret : %d ok\r\n",__key, ret);

	while (values.empty() == false)
	{
		const char *value = values.back();
		values.pop_back();
		delete value;
	}

	return true;
}

static bool test_rpush5(acl::redis_list &redis, int n)
{
	std::vector<const char*> values;
	for (int i = 0; i < n; ++i) 
	{
		char *value = new char[__value_len];
		acl::safe_snprintf(value, __value_len, "%s_%d", __key, i);
		values.push_back(value);
	}

	redis.clear();
	int ret = redis.rpush(__key, values);
	if(ret <= 0)
		printf("rpush key:%s ok",__key);
	else
		printf("rpush key:%s ret : %d ok\r\n",__key, ret);

	while (values.empty() == false)
	{
		const char *value = values.back();
		values.pop_back();
		delete value;
	}

	return ret > 0 ? true : false;
}

static bool test_rpush6(acl::redis_list &redis, int n)
{
	char **values = new char *[n];
	size_t *lens = new size_t[n];
	for (int i = 0; i < n; ++i)
	{
		values[i] = new char[__value_len];
		lens[i] = __value_len;
		acl::safe_snprintf(values[i], __value_len, "%s_%d", __key, i);
	}

	redis.clear();
	int ret = redis.rpush(__key, (const char**) values,lens, n);
	if (ret <= 0)
		printf("rpush key:%s error: %s", __key, redis.result_error());
	else
		printf("rpush key:%s ret : %d ok\r\n", __key, ret);

	DELETE_VALUES(values, n);
	delete []lens;

	return ret > 0 ? true : false;
}

static bool test_lpushx(acl::redis_list& redis, int n)
{
	acl::string value;
	int ret;

	for (int i = 0; i < n; i++)
	{
		value.format("%s_%d", __key, i);
		redis.clear();
		ret = redis.lpushx(__key, value.c_str());
		if (ret <= 0)
		{
			printf("lpushx key: %s error: %s\r\n",
				__key, redis.result_error());
			return false;
		}
		else if (i < max_print_line)
			printf("lpushx ok, key:%s ,value:%s \r\n",
				__key,value.c_str());
	}

	return true;
}

static bool test_lpushx2(acl::redis_list &redis, int n)
{
	acl::string value;
	for (int i = 0; i < n; ++i)
	{
		value.format("%s_%d", __key, i);
		redis.clear();
		int ret = redis.lpushx(__key, value.c_str(), value.length());
		if(ret <= 0)
		{
			printf("lpushx key:%s error: %s\r\n",
				__key, redis.result_error());
			return false;
		}
		else if(i < max_print_line)
			printf("lpushx key:%s ok", __key);
	}

	return true;
	
}

static bool test_rpushx (acl::redis_list &redis, int n)
{
	acl::string value;
	for (int i = 0; i < n; ++i)
	{
		value.format("%s_%d",__key, i);
		redis.clear();
		int ret = redis.rpushx(__key, value.c_str());
		if(ret <= 0)
		{
			printf("lpushx key:%s error: %s\r\n",
				__key, redis.result_error());
			return false;
		}
		else if(i < max_print_line)
			printf("lpushx key:%s ok", __key);
	}

	return true;
}

static bool test_rpushx2 (acl::redis_list &redis, int n)
{
	acl::string value;
	for (int i = 0; i < n; ++i)
	{
		value.format("%s_%d",__key, i);
		redis.clear();
		int ret = redis.rpushx(__key, value.c_str(), value.length());
		if(ret <= 0)
		{
			printf("lpushx key:%s error: %s\r\n",
				__key, redis.result_error());
			return false;
		}
		else if(i < max_print_line)
			printf("lpushx key:%s ok\r\n", __key);
	}

	return true;
}

static bool test_lrange(acl::redis_list& redis)
{
	std::vector<acl::string> result;

	redis.clear();
	bool ret = redis.lrange(__key, 0, 1000, &result);
	if (ret == false)
	{
		printf("lrang key: %s error: %s\r\n",
			__key, redis.result_error());
		return false;
	}

	std::vector<acl::string>::const_iterator citr = result.begin();
	int i = 0;
	printf("lrang key: %s result:\r\n",__key);
	
	for (;citr != result.end(); ++citr)
	{
		printf("%s\r\n",(*citr).c_str());
		if (i >= max_print_line)
			break;
	}

	return true;
}

static bool test_rpop(acl::redis_list &redis, int n)
{
	int ret;
	acl::string buf;

	for (int i = 0; i < n; ++i)
	{
		redis.clear();
		buf.clear();
		ret = redis.rpop(__key, buf);
		if (ret <= 0)
		{
			printf("rpop key: %s error: %s\r\n",
				__key, redis.result_error());
			return false;
		}
		printf("rpop key:%s ,buf:%s ok \r\n", __key, buf.c_str());
	}

	return true;
}

static bool test_lpop(acl::redis_list &redis, int n)
{
	int ret;
	acl::string buf;

	for (int i = 0; i < n; ++i)
	{
		redis.clear();
		buf.clear();
		ret = redis.lpop(__key, buf);
		if (ret <= 0)
		{
			printf("lpop key: %s error: %s\r\n",
				__key, redis.result_error());
			return false;
		}
		printf("lpop key:%s ,buf:%s ok \r\n", __key, buf.c_str());
	}

	return true;
}

static bool test_blpop(acl::redis_list &redis, int n)
{
	std::vector<acl::string> keys;
	acl::string key;
	std::pair<acl::string ,acl::string> result;

	for (int i = 0; i < n; ++i)
	{
		key.format("%s_%d",__key, i);
		keys.push_back(key);
	}
	bool ret = redis.blpop(keys, n, result);
	if (ret == false)
	{
		printf("blpop timeout: %s\r\n", redis.result_error());
		return false;
	}
	else
		printf("blpop key:%s,value:%s ok \r\n",
			result.first.c_str(),result.second.c_str());

	return true;
}

static bool test_brpop(acl::redis_list &redis, int n)
{
	std::vector<acl::string> keys;
	acl::string key;
	std::pair<acl::string ,acl::string> result;

	for (int i = 0; i < n; ++i)
	{
		key.format("%s_%d",__key, i);
		keys.push_back(key);
	}

	redis.clear();
	bool ret = redis.brpop(keys, n, result);
	if (ret == false)
	{
		printf("brpop timeout: %s\r\n", redis.result_error());
		return false;
	}
	else
		printf("brpop key:%s,value:%s ok \r\n",
			result.first.c_str(),result.second.c_str());

	return true;
}

static bool test_rpoplpush(acl::redis_list &redis, int)
{
	acl::string src("list_src");
	acl::string dst("list_dst");
	acl::string buf;

	redis.clear();
	bool ret = redis.rpoplpush(src.c_str(), dst.c_str(), &buf);
	if(ret == false)
	{
		printf("rpoplpush src:%s, dst:%s error: %s\r\n",
			src.c_str(), dst.c_str(), redis.result_error());
		return false;
	}
	else
		printf("rpoplpush src:%s, dst:%s buf:%s ok \r\n",
			src.c_str(), dst.c_str(), buf.c_str());

	return true;

}
static bool test_brpoplpush(acl::redis_list &redis, int)
{
	acl::string src("list1");
	acl::string dst("list");
	acl::string buf;

	redis.clear();
	bool ret = redis.brpoplpush(src.c_str(), dst.c_str(), 1, &buf);
	if(ret == false)
	{
		printf("brpoplpush src:%s, dst:%s timeout: %s\r\n",
			src.c_str(), dst.c_str(), redis.result_error());
		return false;
	}
	else
		printf("rpoplpush src:%s, dst:%s buf:%s ok \r\n",
			src.c_str(), dst.c_str(), buf.c_str());

	return true;
}

static bool test_lrem(acl::redis_list &redis, int n)
{
	redis.clear();
	int ret = redis.lrem(__key, n, "list1_1");
	if (ret < 0)
	{
		printf("lrem key:%s,count:%d error: %s\r\n",
			__key, n, redis.result_error());
		return false;
	}
	else
		printf("lrem key:%s,count :%d ok\r\n", __key, n);

	return true;
}

static bool test_ltrim(acl::redis_list &redis, int n)
{
	redis.clear();
	bool ret = redis.ltrim(__key, 0, n);
	if (ret == false)
	{
		printf("ltrim key:%s,start:%d end:%d error: %s\r\n",
			__key, 0, n, redis.result_error());
		return false;
	}
	else
		printf("ltrim key:%s,start:%d end:%d ok\r\n", __key, 0, n);

	return true;
}

static bool test_llen(acl::redis_list &redis, int)
{
	redis.clear();
	int len = redis.llen(__key);
	if (len < 0)
	{
		printf("llen key:%s error: %s\r\n", __key, redis.result_error());
		return false;
	}
	else
		printf("llen key:%s ret : %d \r\n", __key, len);

	return true;
}

static bool test_lindex(acl::redis_list &redis, int n)
{
	acl::string buf;

	redis.clear();
	bool ret = redis.lindex(__key, n, buf);
	if (ret == false)
	{
		printf("lindex key:%s error: %s\r\n",__key, redis.result_error());
		return false;
	}
	else
		printf("lindex key:%s, value:%s, ok\r\n", __key, buf.c_str());

	return true;
}

static bool test_lset(acl::redis_list &redis, int n)
{
	redis.clear();
	bool ret = redis.lset(__key, n, "new_value");
	if (ret == false)
	{
		printf("lset key:%s ,index:%d error: %s\r\n",
			__key, n, redis.result_error());
		return false;
	}
	else
		printf("lset key:%s ,index:%d ok \r\n",__key, n);

	return true;
}

static bool test_linsert_before(acl::redis_list &redis, int)
{
	redis.clear();
	int ret  = redis.linsert_before(__key, "list1_1","list1_new");
	if (ret < 0)
	{
		printf("linsert_before key:%s, error: %s\r\n",
			__key, redis.result_error());
		return false;
	}
	else
		printf("linsert_before key:%s,ok\r\n", __key);

	return true;
}

static bool test_linsert_after(acl::redis_list &redis, int)
{
	redis.clear();
	int ret  = redis.linsert_after(__key, "list1_1","list1_new");
	if (ret < 0)
	{
		printf("linsert_before key:%s, error: %s\r\n",
			__key, redis.result_error());
		return false;
	}
	else
		printf("linsert_before key:%s,ok\r\n", __key);

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
		"-c [use cluster mode]\r\n"
		"-a cmd[lpush|rpush|lpushx|rpushx|lrange|rpop|lpop|blpop|brpop|rpoplpush|brpoplpush|lrem|ltrim|llen|lindex|lset|linsert_before|linsert_after]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 50, conn_timeout = 10, rw_timeout = 10;
	acl::string addr("127.0.0.1:6379"), cmd("all");
	bool cluster_mode = false;

	acl::log::stdout_open(true);

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

	acl::redis_list redis(&client);
	
	if (cluster_mode)
		redis.set_cluster(&cluster);
	else
		redis.set_client(&client);

	bool ret;

	if (cmd == "lpush")
	{
		ret = test_lpush(redis, n)
			&& test_lpush2(redis, n)
			&& test_lpush3(redis, n)
			&& test_lpush4(redis, n)
			&& test_lpush5(redis, n)
			&& test_lpush6(redis, n);
	}
	else if (cmd == "rpush")
		ret = test_rpush(redis, n)
			&& test_rpush2(redis, n)
			&& test_rpush3(redis, n)
			&& test_rpush4(redis, n)
			&& test_rpush5(redis, n)
			&& test_rpush6(redis, n);
	else if (cmd == "lpushx")
		ret = test_lpushx(redis, n) && test_lpushx2(redis, n);
	else if (cmd == "rpushx")
		ret = test_rpushx(redis, n) && test_rpushx2(redis, n);
	else if (cmd == "lrange")
		ret = test_lrange(redis);
	else if (cmd == "rpop")
		ret = test_rpop(redis, n);
	else if (cmd == "lpop")
		ret = test_lpop(redis, n);
	else if (cmd == "blpop")
		ret = test_blpop(redis, n);
	else if (cmd == "brpop")
		ret = test_brpop(redis, n);
	else if (cmd == "rpoplpush")
		ret = test_rpoplpush(redis, n);
	else if (cmd == "brpoplpush")
		ret = test_brpoplpush(redis, n);
	else if (cmd == "lrem")
		ret = test_lrem(redis, n);
	else if (cmd == "ltrim")
		ret = test_ltrim(redis, n);
	else if (cmd == "llen")
		ret = test_llen(redis, n);
	else if (cmd == "lindex")
		ret = test_lindex(redis, n);
	else if (cmd == "lset")
		ret = test_lset(redis, n);
	else if (cmd == "linsert_before")
		ret = test_linsert_before(redis,1);
	else if (cmd == "linsert_after")
		ret = test_linsert_after(redis,1);
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
