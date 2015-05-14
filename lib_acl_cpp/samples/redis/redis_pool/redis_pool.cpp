#include "stdafx.h"

static acl::string __keypre("test_key");

static bool test_del(acl::redis_key& redis, int i)
{
	acl::string key;

	key.format("%s_%d", __keypre.c_str(), i);
	redis.clear();
	int ret = redis.del(key.c_str(), NULL);
	if (ret < 0)
	{
		printf("del key: %s error\r\n", key.c_str());
		return false;
	}
	else if (i < 10)
		printf("del ok, key: %s\r\n", key.c_str());
	return true;
}

static bool test_expire(acl::redis_key& redis, int i)
{
	acl::string key;

	key.format("%s_%d", __keypre.c_str(), i);
	redis.clear();
	if (redis.expire(key.c_str(), 100) < 0)
	{
		printf("expire key: %s error\r\n", key.c_str());
		return false;
	}
	else if (i < 10)
		printf("expire ok, key: %s\r\n", key.c_str());
	return true;
}

static bool test_ttl(acl::redis_key& redis, int i)
{
	acl::string key;
	int ttl;

	key.format("%s_%d", __keypre.c_str(), i);
	redis.clear();
	if ((ttl = redis.ttl(key.c_str())) < 0)
	{
		printf("get ttl key: %s error\r\n", key.c_str());
		return false;
	}
	else if (i < 10)
		printf("ttl ok, key: %s, ttl: %d\r\n", key.c_str(), ttl);
	return true;
}

static bool test_exists(acl::redis_key& redis, int i)
{
	acl::string key;

	key.format("%s_%d", __keypre.c_str(), i);
	redis.clear();
	if (redis.exists(key.c_str()) == false)
	{
		if (i < 10)
			printf("no exists key: %s\r\n", key.c_str());
	}
	else
	{
		if (i < 10)
			printf("exists key: %s\r\n", key.c_str());
	}
	return true;
}

static bool test_type(acl::redis_key& redis, int i)
{
	acl::string key;

	key.format("%s_%d", __keypre.c_str(), i);
	redis.clear();
	acl::redis_key_t ret = redis.type(key.c_str());
	if (ret == acl::REDIS_KEY_NONE)
	{
		printf("unknown type key: %s\r\n", key.c_str());
		return false;
	}
	else if (i < 10)
		printf("type ok, key: %s, ret: %d\r\n", key.c_str(), ret);
	return true;
}

class test_thread : public acl::thread
{
public:
	test_thread(acl::redis_client_pool& pool, const char* cmd, int n)
		: pool_(pool), cmd_(cmd), n_(n) {}

	~test_thread() {}

protected:
	virtual void* run()
	{
		bool ret;
		acl::redis_client* conn;
		acl::redis_key redis;

		for (int i = 0; i < n_; i++)
		{
			conn = (acl::redis_client*) pool_.peek();
			
			if (conn == NULL)
			{
				printf("peek redis_client failed\r\n");
				break;
			}

			redis.set_client(conn);

			if (cmd_ == "del")
				ret = test_del(redis, i);
			else if (cmd_ == "expire")
				ret = test_expire(redis, i);
			else if (cmd_ == "ttl")
				ret = test_ttl(redis, i);
			else if (cmd_ == "exists")
				ret = test_exists(redis, i);
			else if (cmd_ == "type")
				ret = test_type(redis, i);
			else if (cmd_ == "all")
			{
				if (test_expire(redis, i) == false
					|| test_ttl(redis, i) == false
					|| test_exists(redis, i) == false
					|| test_type(redis, i) == false
					|| test_del(redis, i) == false)
				{
					ret = false;
				}
				else
					ret = true;
			}
			else
			{
				printf("unknown cmd: %s\r\n", cmd_.c_str());
				break;
			}
			pool_.put(conn, ret);
			if (ret == false)
				break;
		}

		return NULL;
	}

private:
	acl::redis_client_pool& pool_;
	acl::string cmd_;
	int n_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s redis_addr[127.0.0.1:6379]\r\n"
		"-n count[default: 10]\r\n"
		"-C connect_timeout[default: 10]\r\n"
		"-I rw_timeout[default: 10]\r\n"
		"-c max_threads[default: 10]\r\n"
		"-a cmd[expire|ttl|exists|type|del]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 1, conn_timeout = 10, rw_timeout = 10;
	int  max_threads = 10;
	acl::string addr("127.0.0.1:6379"), cmd;

	while ((ch = getopt(argc, argv, "hs:n:C:I:c:a:")) > 0)
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
		default:
			break;
		}
	}

	acl::acl_cpp_init();

	acl::redis_client_pool pool(addr.c_str(), max_threads);
	pool.set_timeout(conn_timeout, rw_timeout);

	std::vector<test_thread*> threads;
	for (int i = 0; i < max_threads; i++)
	{
		test_thread* thread = new test_thread(pool, cmd.c_str(), n);
		threads.push_back(thread);
		thread->set_detachable(false);
		thread->start();
	}

	std::vector<test_thread*>::iterator it = threads.begin();
	for (; it != threads.end(); ++it)
	{
		(*it)->wait();
		delete (*it);
	}

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
