#include "stdafx.h"

/**
 * author: niukey@qq.com
 * date: 2015.1.27
 * modify by zsx
 */

//static acl::string __keypre("list");
static const char *__key = "list";
static const int max_print_line = 50;
static const int __value_len = 32;

#define DELETE_VALUES(values,n)\
do\
{  for(int i = 0; i < (n) ; ++i)\
	delete (values)[i];\
   delete (values);\
} while (0);


static void test_lpush(acl::redis_list& option, int n)
{
	acl::string value;

	for (int i = 0; i < n; i++)
	{
		value.format("%s_%d", __key, i);
		option.reset();
		int ret = option.lpush(__key, value.c_str(),NULL);
		if (ret <= 0)
		{
			printf("lpush key: %s error\r\n", __key);
			break;
		}
		else if (i < max_print_line)
			printf("lpush ok, key:%s ,value:%s \r\n",
				__key,value.c_str());
	}
}
//int lpush(const char* key, const char* values[], size_t argc);

static void test_lpush2(acl::redis_list& option, int n)
{
	char** value = new char *[n+1];
	for (int i = 0; i < n; i++)
	{
		value[i] = new char[__value_len];
		acl::safe_snprintf(value[i], __value_len,"%s_%d",__key,i);
	}
	value[n] = NULL;

	option.reset();
	int ret = option.lpush(__key, (const char**) value, n);
	if (ret <= 0)
		printf("lpush key: %s error\r\n", __key);
	else
		printf("lpush key:%s ret : %d ok\r\n",__key, ret);
	for (int i = 0; i < n; ++i) {
		delete value[i];
	}
	delete [] value;
}
//int lpush(const char* key, const std::vector<string>& values);
static void test_lpush3(acl::redis_list &option, int n)
{
	std::vector<acl::string> values;
	acl::string buf;
	for (int i = 0; i < n; ++i) {
		buf.format("%s_%d",__key, i);
		values.push_back(buf);
	}
	int ret = option.lpush(__key, values);
	if (ret <= 0)
		printf("lpush key:%s error\r\n",__key);
	else
		printf("lpush key:%s ret : %d ok\r\n",__key, ret);
}

//nt lpush(const char* key, const std::vector<char*>& values);
static void test_lpush4(acl::redis_list &option, int n)
{
	std::vector<const char *> values;
	char **value  = new char *[n];

	for (int i  = 0; i < n; ++i) {
		value[i] = new char[__value_len];
		acl::safe_snprintf(value[i], __value_len, "%s_%d", __key, i);
		values.push_back(value[i]);
	}
	int ret = option.lpush(__key, (std::vector<const char*>&) values);
	if(ret <= 0)
		printf("lpush key:%s error\r\n",__key);
	else
		printf("lpush key:%s ret : %d ok\r\n",__key, ret);

	for (int i = 0; i < n; ++i) {
		delete value[i];
	}
	delete [] value;
}
//int lpush(const char* key, const std::vector<const char*>& values);
static void test_lpush5(acl::redis_list &option, int n)
{
	std::vector<const char*> values;
	char **value  = new char *[n];
	for (int i  = 0; i < n; ++i) {
		value[i] = new char[__value_len];
		acl::safe_snprintf(value[i], __value_len, "%s_%d", __key, i);
		values.push_back(value[i]);
	}
	int ret = option.lpush(__key, values);
	if(ret <= 0)
		printf("lpush key:%s error\r\n",__key);
	else
		printf("lpush key:%s ret : %d ok\r\n",__key, ret);

	for (int i = 0; i < n; ++i) {
		delete value[i];
	}
	delete [] value;
}

//int lpush(const char* key, const char* values[], size_t lens[],size_t argc);
static void test_lpush6(acl::redis_list &option, int n)
{
	char **values = new char *[n];
	size_t *lens = new size_t[n];
	for (int i = 0; i < n ;++i)
	{
		values[i] = new char [__value_len];
		acl::safe_snprintf(values[i],__value_len, "%s_%d", __key, i);
		lens[i] = strlen(values[i]);
	}
	int ret = option.lpush(__key, (const char**)values, lens, n);
	
	if (ret <= 0)
		printf("lpush key:%s error\r\n", __key);
	else
		printf("lpush key:%s ret : %d ok\r\n",__key, ret);

	for (int i = 0; i < n; ++i) {
		delete values[i];
	}
	delete [] values;
}
//int rpush(const char* key, const char* first_value, ...);
static void test_rpush(acl::redis_list& option, int n)
{
	acl::string value;
	int ret;
	for (int i = 0; i < n; ++i) {
		value.format("%s_%d",__key, i);
		option.reset();
		ret = option.rpush(__key, value.c_str(), NULL);
		if (ret <= 0) {
			printf("rpush key:%s error, \r\n",__key);
			break;
		}else if (i < max_print_line) {
			printf("rpush key:%s, value:%s ok \r\n",
				__key, value.c_str());
		}
	}
}
//int rpush(const char* key, const char* values[], size_t argc);
static void  test_rpush2(acl::redis_list& option, int n)
{
	char **values = new char *[n];
	for (int i = 0 ; i < n ; ++ i)
	{
		values[i] = new char[__value_len];
		acl::safe_snprintf(values[i], __value_len, "%s_%d",__key, i);
	}
	int ret = option.rpush(__key, (const char **)values, n);
	if(ret <= 0)
		printf("rpush key:%s error\r\n",__key);
	else
		printf("rpush key:%s ret : %d ok\r\n",__key, ret);

	DELETE_VALUES(values, n);
}
//int rpush(const char* key, const std::vector<string>& values);
static void test_rpush3(acl::redis_list &option, int n)
{
	std::vector<acl::string> values;
	acl::string value;
	for (int i = 0; i < n; ++i)
	{
		value.format("%s_%d",__key, i);
		values.push_back(value);
	}
	int ret = option.rpush(__key, values);
	if(ret <= 0)
		printf("rpush key:%s error",__key);
	else
		printf("rpush key:%s ret : %d ok\r\n",__key, ret);
}
//int rpush(const char* key, const std::vector<char*>& values);
static void test_rpush4(acl::redis_list &option, int n)
{
	std::vector<const char*> values;
	for (int i = 0; i < n; ++i) 
	{
		char *value = new char[__value_len];
		acl::safe_snprintf(value, __value_len, "%s_%d", __key, i);
		values.push_back(value);
	}
	int ret = option.rpush(__key, (const std::vector<const char*>&) values);
	if(ret <= 0)
		printf("rpush key:%s ok",__key);
	else
		printf("rpush key:%s ret : %d ok\r\n",__key, ret);

	while (values.empty() == false){
		const char *value = values.back();
		values.pop_back();
		delete value;
	}
}
//int rpush(const char* key, const std::vector<const char*>& values);
static void test_rpush5(acl::redis_list &option, int n)
{
	std::vector<const char*> values;
	for (int i = 0; i < n; ++i) 
	{
		char *value = new char[__value_len];
		acl::safe_snprintf(value, __value_len, "%s_%d", __key, i);
		values.push_back(value);
	}
	int ret = option.rpush(__key, values);

	if(ret <= 0)
		printf("rpush key:%s ok",__key);
	else
		printf("rpush key:%s ret : %d ok\r\n",__key, ret);

	while (values.empty() == false){
		const char *value = values.back();
		values.pop_back();
		delete value;
	}
}
//int rpush(const char* key, const char* values[], size_t lens[],size_t argc);
static void test_rpush6(acl::redis_list &option, int n)
{
	char **values = new char *[n];
	size_t *lens = new size_t[n];
	for (int i = 0; i < n; ++i)
	{
		values[i] = new char[__value_len];
		lens[i] = __value_len;
		acl::safe_snprintf(values[i], __value_len, "%s_%d", __key, i);
	}
	int ret = option.rpush(__key, (const char**) values,lens, n);
	if(ret <= 0)
		printf("rpush key:%s error",__key);
	else
		printf("rpush key:%s ret : %d ok\r\n",__key, ret);

	DELETE_VALUES(values, n);
	delete []lens;
}
static void test_lpushx(acl::redis_list& option, int n)
{
	acl::string value;
	int ret;

	for (int i = 0; i < n; i++) {
		value.format("%s_%d", __key, i);
		option.reset();
		ret = option.lpushx(__key, value.c_str());
		if (ret <= 0) {
			printf("lpushx key: %s error\r\n", __key);
			break;
		}
		else if (i < max_print_line)
			printf("lpushx ok, key:%s ,value:%s \r\n",
				__key,value.c_str());
	}
}
//int lpushx(const char* key, const char* value, size_t len);
static void test_lpushx2(acl::redis_list &option, int n)
{
	acl::string value;
	for (int i = 0; i < n; ++i)
	{
		value.format("%s_%d",__key, i);
		option.reset();
		int ret = option.lpushx(__key, value.c_str(),value.length());
		if(ret <= 0){
			printf("lpushx key:%s error \r\n", __key);
			break;
		}
		else if(i < max_print_line)
			printf("lpushx key:%s ok",__key);
	}
	
}
//int rpushx(const char* key, const char* value);
static void test_rpushx (acl::redis_list &option, int n)
{
	acl::string value;
	for (int i = 0; i < n; ++i)
	{
		value.format("%s_%d",__key, i);
		option.reset();
		int ret = option.rpushx(__key, value.c_str());
		if(ret <= 0){
			printf("lpushx key:%s error \r\n", __key);
			break;
		}
		else if(i < max_print_line)
			printf("lpushx key:%s ok",__key);
	}

}
//int rpushx(const char* key, const char* value, size_t len);
static void test_rpushx2 (acl::redis_list &option, int n)
{
	acl::string value;
	for (int i = 0; i < n; ++i)
	{
		value.format("%s_%d",__key, i);
		option.reset();
		int ret = option.rpushx(__key, value.c_str(), value.length());
		if(ret <= 0){
			printf("lpushx key:%s error \r\n", __key);
			break;
		}
		else if(i < max_print_line)
			printf("lpushx key:%s ok\r\n",__key);
	}

}
static void test_lrange(acl::redis_list& option)
{
	std::vector<acl::string> result;

	bool ret = option.lrange(__key, 0, 1000, &result);
	if (ret == false) {
		printf("lrang key: %s error\r\n",__key);
		return;
	}

	std::vector<acl::string>::const_iterator citr = result.begin();
	int i = 0;
	printf("lrang key: %s result:\r\n",__key);
	
	for (;citr != result.end(); ++citr) {
		printf("%s\r\n",(*citr).c_str());
		if (i >= max_print_line)
			break;
	}
}
//int rpop(const char* key, string& buf);
static void test_rpop(acl::redis_list &option, int n)
{
	int ret;
	acl::string buf;
	for (int i = 0; i < n; ++i) {
		option.reset();
		buf.clear();
		ret = option.rpop(__key, buf);
		if (ret <= 0) {
			printf("rpop key: %s error\r\n",__key);
			break;
		}
		printf("rpop key:%s ,buf:%s ok \r\n",__key, buf.c_str());
	}
}
//int lpop(const char* key, string& buf);
static void test_lpop(acl::redis_list &option, int n)
{
	int ret;
	acl::string buf;
	for (int i = 0; i < n; ++i) {
		option.reset();
		buf.clear();
		ret = option.lpop(__key, buf);
		if (ret <= 0) {
			printf("lpop key: %s error\r\n",__key);
			break;
		}
		printf("lpop key:%s ,buf:%s ok \r\n",__key, buf.c_str());
	}
}
/*bool blpop(const std::vector<string>& keys, size_t timeout,
std::pair<string, string>& result);*/
static void test_blpop(acl::redis_list &option, int n)
{
	std::vector<acl::string> keys;
	acl::string key;
	std::pair<acl::string ,acl::string> result;

	for (int i = 0; i < n; ++i)
	{
		key.format("%s_%d",__key, i);
		keys.push_back(key);
	}
	bool ret = option.blpop(keys, n, result);
	if (ret == false)
		printf("blpop timeout\r\n");
	else
		printf("blpop key:%s,value:%s ok \r\n",
			result.first.c_str(),result.second.c_str());
}
/*bool brpop(const std::vector<string>& keys, size_t timeout,
std::pair<string, string>& result);*/
static void test_brpop(acl::redis_list &option, int n)
{
	std::vector<acl::string> keys;
	acl::string key;
	std::pair<acl::string ,acl::string> result;

	for (int i = 0; i < n; ++i)
	{
		key.format("%s_%d",__key, i);
		keys.push_back(key);
	}
	bool ret = option.brpop(keys, n, result);
	if (ret == false)
		printf("brpop timeout\r\n");
	else
		printf("brpop key:%s,value:%s ok \r\n",
		result.first.c_str(),result.second.c_str());
}
//bool rpoplpush(const char* src, const char* dst, string* buf = NULL);
static void test_rpoplpush(acl::redis_list &option, int)
{
	acl::string src("list_src");
	acl::string dst("list_dst");
	acl::string buf;
	bool ret = option.rpoplpush(src.c_str(), dst.c_str(), &buf);
	if(ret == false)
		printf("rpoplpush src:%s, dst:%s error\r\n",
			src.c_str(), dst.c_str());
	else
		printf("rpoplpush src:%s, dst:%s buf:%s ok \r\n",
			src.c_str(), dst.c_str(), buf.c_str());

}
/*bool brpoplpush(const char* src, const char* dst, size_t timeout,
				string* buf = NULL);*/
static void test_brpoplpush(acl::redis_list &option, int)
{
	acl::string src("list1");
	acl::string dst("list");
	acl::string buf;
	bool ret = option.brpoplpush(src.c_str(), dst.c_str(), 1, &buf);
	if(ret == false)
		printf("brpoplpush src:%s, dst:%s timeout\r\n",src.c_str(), 
			dst.c_str());
	else
		printf("rpoplpush src:%s, dst:%s buf:%s ok \r\n",
			src.c_str(), dst.c_str(), buf.c_str());
}

//int lrem(const char* key, int count, const char* value);
void static test_lrem(acl::redis_list &option, int n)
{
	int ret = option.lrem(__key, n, "list1_1");
	if (ret <= 0)
		printf("lrem key:%s,count:%d error\r\n",__key, n);
	else
		printf("lrem key:%s,count :%d ok\r\n", __key, n);
}
//bool ltrim(const char* key, size_t start, size_t end);
void static test_ltrim(acl::redis_list &option, int n)
{
	bool ret = option.ltrim(__key, 0, n);
	if (ret == false)

		printf("ltrim key:%s,start:%d end:%d error\r\n",__key, 0, n);
	else
		printf("ltrim key:%s,start:%d end:%d ok\r\n",__key, 0, n);
}
//llen(const char *)
void static test_llen(acl::redis_list &option, int)
{
	int len = option.llen(__key);
	if (len < 0)
		printf("llen key:%s error\r\n", __key);
	else
		printf("llen key:%s ret : %d \r\n",__key, len);
}
/*bool lindex(const char* key, size_t idx, string& buf,
bool* exist = NULL);*/
void static test_lindex(acl::redis_list &option, int n)
{
	acl::string buf;
	bool ret = option.lindex(__key, n, buf);
	if(ret == false)
		printf("lindex key:%s error\r\n",__key);
	else
		printf("lindex key:%s, value:%s, ok\r\n", __key, buf.c_str());
}
/*bool lset(const char* key, size_t idx, const char* value);*/
void static test_lset(acl::redis_list &option, int n)
{
	bool ret = option.lset(__key, n, "new_value");
	if(ret == false)
		printf("lset key:%s ,index:%d error\r\n",__key, n);
	else
		printf("lset key:%s ,index:%d ok \r\n",__key, n);
}
/*int linsert_before(const char* key, const char* pivot,
const char* value);*/
void static test_linsert_before(acl::redis_list &option, int)
{
	int ret  = option.linsert_before(__key, "list1_1","list1_new");
	if(ret < 0 )
		printf("linsert_before key:%s,error\r\n",__key);
	else
		printf("linsert_before key:%s,ok\r\n",__key);
}
/*int linsert_after(const char* key, const char* pivot,
const char* value);*/
void static test_linsert_after(acl::redis_list &option, int)
{
	int ret  = option.linsert_after(__key, "list1_1","list1_new");
	if(ret < 0 )
		printf("linsert_before key:%s,error\r\n",__key);
	else
		printf("linsert_before key:%s,ok\r\n",__key);
}
static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s redis_addr[127.0.0.1:6379]\r\n"
		"-n count\r\n"
		"-C connect_timeout[default: 10]\r\n"
		"-I rw_timeout[default: 10]\r\n"
		"-S [if slice request, default: no]\r\n"
		"-a cmd[lpush|rpush|lpushx|rpushx|lrange|rpop|lpop|blpop|brpop|rpoplpush|brpoplpush|lrem|ltrim|llen|lindex|lset|linsert_before|linsert_after]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 50, conn_timeout = 10, rw_timeout = 10;
	acl::string addr("127.0.0.1:6379"), cmd("all");

	while ((ch = getopt(argc, argv, "hs:n:C:T:a:")) > 0)
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
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::redis_client client(addr.c_str(), conn_timeout, rw_timeout);
	acl::redis_list option(&client);
	
	if (cmd == "lpush")
	{
		test_lpush(option, n);
		test_lpush2(option, n);
		test_lpush3(option, n);
		test_lpush4(option, n);
		test_lpush5(option, n);
		test_lpush6(option, n);
	}
	else if (cmd == "rpush")
	{
		test_rpush(option, n);
		test_rpush2(option, n);
		test_rpush3(option, n);
		test_rpush4(option, n);
		test_rpush5(option, n);
		test_rpush6(option, n);
	}
	else if (cmd == "lpushx")
	{
		test_lpushx(option, n);
		test_lpushx2(option, n);
	}
	else if (cmd == "rpushx")
	{
		test_rpushx(option, n);
		test_rpushx2(option, n);
	}
	else if (cmd == "lrange")
		test_lrange(option);
	else if (cmd == "rpop")
		test_rpop(option, n);
	else if (cmd == "lpop")
		test_lpop(option, n);
	else if (cmd == "blpop")
		test_blpop(option, n);
	else if (cmd == "brpop")
		test_brpop(option, n);
	else if (cmd == "rpoplpush")
		test_rpoplpush(option, n);
	else if (cmd == "brpoplpush")
		test_brpoplpush(option, n);
	else if (cmd == "lrem")
		test_lrem(option, n);
	else if (cmd == "ltrim")
		test_ltrim(option, n);
	else if (cmd == "llen")
		test_llen(option, n);
	else if (cmd == "lindex")
		test_lindex(option, n);
	else if (cmd == "lset")
		test_lset(option, n);
	else if (cmd == "linsert_before")
		test_linsert_before(option,1);
	else if (cmd == "linsert_after")
		test_linsert_after(option,1);
	
#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif

	return 0;
}
