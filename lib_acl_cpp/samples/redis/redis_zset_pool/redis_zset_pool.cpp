#include "stdafx.h"

static bool test_zadd(acl::redis_zset& redis, int i, const char* key,
	const char* big_data, size_t length, size_t base_length)
{
	// 将大数据进行分割，计算出分割后的数据块个数
	size_t nmember = length / base_length;
	if (length % base_length != 0)
		nmember++;

	// 从连接对象中获得统一的内存池分配对象，分配小内存块
	acl::dbuf_pool* pool = redis.get_dbuf();
	// 动态分配数据块指针数组内存
	const char** members = (const char**)
		pool->dbuf_alloc(nmember * sizeof(char*));
	// 动态分配数据块长度数组内存
	size_t* lens = (size_t*) pool->dbuf_alloc(nmember * sizeof(size_t));
	// 动态分配数据块分值数组内存
	double* scores = (double*) pool->dbuf_alloc(nmember * sizeof(double));

	// 将大数据切分成小数据，置入数据块数组中，使用递增的整数做为分值
	size_t len;
	const char* ptr = big_data;
	char* buf, id[64];
	int n;

	for (size_t j = 0; j < nmember; j++)
	{
		len = length > base_length ? base_length : length;

		// 在每个原始数据前面加唯一前缀，从而可以保证有序集合中对象中
		// 的每个成员数据都是不同的
		n = acl::safe_snprintf(id, sizeof(id),
			"%lu:", (unsigned long) j);
		buf = (char*) pool->dbuf_alloc(len + n);
		memcpy(buf, id, n);
		memcpy(buf + n, ptr, len);
		members[j] = buf;

		lens[j] = len + n; // 该数据块的总长度：唯一前缀+数据
		scores[j] = (double) j;

		// 剩余数据块长度
		length -= len;
		ptr += len;
	}

	// 要求 redis 连接对象采用内存链协议组装方式，避免内部组装请求协议时
	// 再组装成大内存
	redis.get_client()->set_slice_request(true);

	// 开始向 redis 添加数据
	int ret = redis.zadd(key, members, lens, scores, nmember);
	if (ret < 0)
	{
		printf("add key: %s error\r\n", key);
		return false;
	}
	else if (i < 10)
		printf("add ok, key: %s, ret: %d\r\n", key, ret);

	return true;
}

static bool test_zcard(acl::redis_zset& redis, int i, const char* key)
{
	// 因为该协议数据比较小，所以在组装请求数据时不必采用分片方式
	redis.get_client()->set_slice_request(false);

	int ret = redis.zcard(key);
	if (ret < 0)
	{
		printf("zcard key: %s error\r\n", key);
		return false;
	}
	else if (i < 10)
		printf("zcard ok, key: %s, count: %d\r\n", key, ret);

	return true;
}

static bool test_zrange(acl::redis_zset& redis, int i, const char* key,
	const char* hmac)
{
	int start = 0, end = -1;

	// 请求的数据量比较小，所以在组装请求协议时不必采用分片方式
	redis.get_client()->set_slice_request(false);

	// 对服务器返回的数据也不分片
	redis.get_client()->set_slice_respond(false);

	int ret = redis.zrange(key, start, end, NULL);
	if (ret <= 0)
	{
		printf("zrange return: %d\r\n", ret);
		return false;
	}

	// 获得数组元素结果集
	const acl::redis_result* result = redis.get_result();
	if (result == NULL)
	{
		printf("result null\r\n");
		return false;
	}

	size_t size;
	// 直接获得数组集合
	const acl::redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0)
	{
		printf("no children: %s, size: %d\r\n",
			children ? "no" : "yes", (int) size);
		return false;
	}

	// 校验获得的所有数据片的 MD5 值，与传入的进行比较
	acl::md5* md5;
	if (hmac != NULL)
		md5 = new acl::md5;
	else
		md5 = NULL;

	const acl::redis_result* child;
	size_t len, argc, n;

	// 先遍历所有数组元素对象
	for (size_t j = 0; j < size; j++)
	{
		child = children[j];
		if (child == NULL)
			continue;

		// 因为前面设置了禁止对响应数据进行分片，所以只需取第一个元素
		argc = child->get_size();
		assert(argc == 1);

		const char* ptr = child->get(0, &len);
		if (ptr == NULL)
		{
			printf("first is null\r\n");
			continue;
		}

		const char* dat = strchr(ptr, ':');
		if (dat == NULL)
		{
			printf("invalid data, j: %d\n", (int) j);
			continue;
		}
		dat++;
		n = dat - ptr;
		if (len < n)
		{
			printf("invalid data, j: %d\n", (int) j);
			continue;
		}

		len -= n;

		// 取出数据计算 md5 值
		if (md5 != NULL)
			md5->update(dat, len);
	}

	if (md5 != NULL)
		md5->finish();

	// 获得字符串方式的 MD5 值
	if (md5 != NULL)
	{
		const char* ptr = md5->get_string();
		if (strcmp(ptr, hmac) != 0)
		{
			printf("md5 error, hmac: %s, %s, key: %s\r\n",
				hmac, ptr, key);
			return false;
		}
		else if (i < 10)
			printf("md5 ok, hmac: %s, %s, key: %s\r\n",
				hmac, ptr, key);
		delete md5;
	}
	else if (i < 10)
		printf("ok, key: %s\r\n", key);

	return true;
}

static bool test_del(acl::redis_key& redis, int i, const char* key)
{
	int ret = redis.del(key) < 0 ? false : true;
	if (ret < 0)
		printf("del %s error, i: %d\r\n", key, i);
	else if (i < 10)
		printf("del %s ok, i: %d\r\n", key, i);
	return ret >= 0 ? true : false;
}

/////////////////////////////////////////////////////////////////////////////

static acl::string __keypre("zset_key");
static size_t __base_length = 8192;  // 基准数据块长度
static char* __big_data;
static size_t __big_data_length = 10240000;  // 大数据块长度，默认是 10 MB
static char* __hmac;

// 子线程类，每个线程对象与 redis-server 之间建立一个连接
class test_thread : public acl::thread
{
public:
	test_thread(acl::redis_client_pool& pool, const char* cmd, int n, int id)
		: pool_(pool), cmd_(cmd), n_(n), id_(id) {}

	~test_thread() {}

protected:
	virtual void* run()
	{
		bool ret;
		acl::redis_client* conn;
		acl::redis_zset redis;
		acl::redis_key key_redis;
		acl::string key;

		for (int i = 0; i < n_; i++)
		{
			// 从全局线程池中获取一个 redis 连接对象
			conn = (acl::redis_client*) pool_.peek();
			
			if (conn == NULL)
			{
				printf("peek redis_client failed\r\n");
				break;
			}

			// 每个线程一个 ID 号，做为键值组成部分
			key.format("%s_%d_%d", __keypre.c_str(), id_, i);

			redis.clear();
			// 将 redis 连接对象与 redis 命令操作类对象进行绑定关联
			redis.set_client(conn);

			if (cmd_ == "zadd")
				ret = test_zadd(redis, i, key.c_str(),
					__big_data, __big_data_length,
					__base_length);
			else if (cmd_ == "zcard")
				ret = test_zcard(redis, i, key);
			else if (cmd_ == "zrange")
				ret = test_zrange(redis, i, key, __hmac);
			else if (cmd_ == "del")
			{
				key_redis.set_client(conn);
				ret = test_del(key_redis, i, key);
			}
			else if (cmd_ != "all")
			{
				printf("unknown cmd: %s\r\n", cmd_.c_str());
				ret = false;
			}
			else if (test_zadd(redis, i, key.c_str(),
					__big_data, __big_data_length,
					__base_length) == false
				|| test_zcard(redis, i, key) == false
				|| test_zrange(redis, i, key, __hmac) == false)
			{
				ret = false;
			}
			else
				ret = true;

			// 将 redis 连接对象归还给连接池，是否保持该连接，
			// 通过判断该连接是否断开决定
			pool_.put(conn, !conn->eof());

			if (ret == false)
				break;
		}

		return NULL;
	}

private:
	acl::redis_client_pool& pool_;
	acl::string cmd_;
	int n_;
	int id_;
};

static void init(const char* cmd, bool check)
{
	if (strcasecmp(cmd, "zrange") == 0 && check == false)
		return;

	acl::md5 md5;
	char ch;

	__big_data = (char*) malloc(__big_data_length);
	for (size_t i = 0; i < __big_data_length; i++)
	{
		ch = (char) i % 255;
		__big_data[i] = ch;
		md5.update(&ch, 1);
	}

	//md5.update(__big_data, __big_data_length);
	md5.finish();

	__hmac = (char*) malloc(33);
	acl::safe_snprintf(__hmac, 33, "%s", md5.get_string());

	printf("init ok, hmac: %s, length: %lu, base: %lu, slice: %d\r\n",
		__hmac, (unsigned long) __big_data_length,
		(unsigned long) __base_length,
		(int) __big_data_length / __base_length
			+ __big_data_length % __base_length == 0 ? 0 : 1);

	md5.reset();
	md5.update(__big_data, __big_data_length);
	md5.finish();
	printf("md5 once: %s\r\n", md5.get_string());
}

static void end()
{
	if (__big_data)
		free(__big_data);
	if (__hmac)
		free(__hmac);
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s redis_addr[127.0.0.1:6379]\r\n"
		"-n count[default: 10]\r\n"
		"-C connect_timeout[default: 10]\r\n"
		"-I rw_timeout[default: 10]\r\n"
		"-c max_threads[default: 10]\r\n"
		"-l max_data_length\r\n"
		"-b base_length\r\n"
		"-S [if check data when cmd is zrange]\r\n"
		"-a cmd[zadd|zcard|zrange|del]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 1, conn_timeout = 10, rw_timeout = 10;
	int  max_threads = 10;
	bool check = false;
	acl::string addr("127.0.0.1:6379"), cmd;

	while ((ch = getopt(argc, argv, "hs:n:C:I:c:a:l:b:S")) > 0)
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
		case 'c':
			max_threads = atoi(optarg);
			break;
		case 'a':
			cmd = optarg;
			break;
		case 'l':
			__big_data_length = (unsigned long) atol(optarg);
			break;
		case 'b':
			__base_length = (size_t) atol(optarg);
			break;
		case 'S':
			check = true;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();

	init(cmd, check);

	acl::redis_client_pool pool(addr.c_str(), max_threads);
	pool.set_timeout(conn_timeout, rw_timeout);

	std::vector<test_thread*> threads;

	// 创建一组线程，每一个线程与 redis-server 建立一个连接
	for (int i = 0; i < max_threads; i++)
	{
		test_thread* thread = new test_thread(pool, cmd.c_str(),
			n, i);
		threads.push_back(thread);
		// 取消线程的分离模式，以便于下面回收线程，等待线程退出
		thread->set_detachable(false);
		thread->start();
	}

	// 回收所有线程
	std::vector<test_thread*>::iterator it = threads.begin();
	for (; it != threads.end(); ++it)
	{
		// 等待某个线程退出
		(*it)->wait();
		delete (*it);
	}

	end();

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif

	return 0;
}
