#include "stdafx.h"

static acl::string __keypre("test_key_cluster");

static bool test_del(acl::redis_key& option, int i)
{
	acl::string key;

	key.format("%s_%d", __keypre.c_str(), i);
	option.reset();
	int ret = option.del(key.c_str(), NULL);
	if (ret < 0)
	{
		printf("del key: %s error\r\n", key.c_str());
		return false;
	}
	else if (i < 10)
		printf("del ok, key: %s\r\n", key.c_str());
	return true;
}

static bool test_expire(acl::redis_key& option, int i)
{
	acl::string key;

	key.format("%s_%d", __keypre.c_str(), i);
	option.reset();
	if (option.expire(key.c_str(), 100) < 0)
	{
		printf("expire key: %s error\r\n", key.c_str());
		return false;
	}
	else if (i < 10)
		printf("expire ok, key: %s\r\n", key.c_str());
	return true;
}

static bool test_ttl(acl::redis_key& option, int i)
{
	acl::string key;
	int ttl;

	key.format("%s_%d", __keypre.c_str(), i);
	option.reset();
	if ((ttl = option.ttl(key.c_str())) < 0)
	{
		printf("get ttl key: %s error\r\n", key.c_str());
		return false;
	}
	else if (i < 10)
		printf("ttl ok, key: %s, ttl: %d\r\n", key.c_str(), ttl);
	return true;
}

static bool test_exists(acl::redis_key& option, int i)
{
	acl::string key;

	key.format("%s_%d", __keypre.c_str(), i);
	option.reset();
	if (option.exists(key.c_str()) == false)
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

static bool test_type(acl::redis_key& option, int i)
{
	acl::string key;

	key.format("%s_%d", __keypre.c_str(), i);
	option.reset();
	acl::redis_key_t ret = option.type(key.c_str());
	if (ret == acl::REDIS_KEY_UNKNOWN)
	{
		printf("unknown type key: %s\r\n", key.c_str());
		return false;
	}
	else if (i < 10)
		printf("type ok, key: %s, ret: %d\r\n", key.c_str(), ret);
	return true;
}

static bool test_set(acl::redis_string& option, int i)
{
	acl::string key;
	key.format("%s_%d", __keypre.c_str(), i);

	acl::string value;
	value.format("value_%s", key.c_str());

	option.reset();
	bool ret = option.set(key.c_str(), value.c_str());
	printf("set key: %s, value: %s %s\r\n", key.c_str(),
		value.c_str(), ret ? "ok" : "error");
	return ret;
}

class test_thread : public acl::thread
{
public:
	test_thread(acl::redis_cluster& cluster, const char* cmd, int n)
		: cluster_(cluster), cmd_(cmd), n_(n) {}

	~test_thread() {}

protected:
	virtual void* run()
	{
		bool ret;
		acl::redis_key option;
		acl::redis_string string_option;

		for (int i = 0; i < n_; i++)
		{
			option.set_cluster(&cluster_);
			string_option.set_cluster(&cluster_);

			if (cmd_ == "set")
				ret = test_set(string_option, i);
			else if (cmd_ == "del")
				ret = test_del(option, i);
			else if (cmd_ == "expire")
				ret = test_expire(option, i);
			else if (cmd_ == "ttl")
				ret = test_ttl(option, i);
			else if (cmd_ == "exists")
				ret = test_exists(option, i);
			else if (cmd_ == "type")
				ret = test_type(option, i);
			else if (cmd_ == "all")
			{
				if (test_expire(option, i) == false
					|| test_ttl(option, i) == false
					|| test_exists(option, i) == false
					|| test_type(option, i) == false
					|| test_del(option, i) == false)
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

			if (ret == false)
			{
				printf("cmd: %s error\r\n", cmd_.c_str());
				break;
			}
		}

		return NULL;
	}

private:
	acl::redis_cluster& cluster_;
	acl::string cmd_;
	int n_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s redis_addr_list[127.0.0.1:6379, 127.0.0.1:6380]\r\n"
		"-n count[default: 10]\r\n"
		"-C connect_timeout[default: 10]\r\n"
		"-I rw_timeout[default: 10]\r\n"
		"-c max_threads[default: 10]\r\n"
		"-a cmd[set|expire|ttl|exists|type|del]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 1, conn_timeout = 10, rw_timeout = 10;
	int  max_threads = 10;
	acl::string addrs("127.0.0.1:6379, 127.0.0.1:6380"), cmd;

	while ((ch = getopt(argc, argv, "hs:n:C:I:c:a:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addrs = optarg;
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
	//acl::log::stdout_open(true);

	acl::redis_cluster cluster(conn_timeout, rw_timeout);
	cluster.init(NULL, addrs.c_str(), max_threads);

	std::vector<test_thread*> threads;
	for (int i = 0; i < max_threads; i++)
	{
		test_thread* thread = new test_thread(cluster, cmd.c_str(), n);
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
