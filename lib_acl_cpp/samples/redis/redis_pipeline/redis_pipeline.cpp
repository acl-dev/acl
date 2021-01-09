#include "stdafx.h"
#include "util.h"

static acl::string __keypre("test_key_cluster");

static bool test_del(acl::redis_key& cmd, int i)
{
	cmd.clear();

	acl::string key;

	key.format("%s_%d", __keypre.c_str(), i);
	int ret = cmd.del(key.c_str());
	if (ret < 0) {
		printf("del key: %s error: %s\r\n",
			key.c_str(), cmd.result_error());
		return false;
	} else if (i < 10) {
		printf("del ok, key: %s\r\n", key.c_str());
	}
	return true;
}

static bool test_expire(acl::redis_key& cmd, int i)
{
	cmd.clear();

	acl::string key;

	key.format("%s_%d", __keypre.c_str(), i);
	if (cmd.expire(key.c_str(), 100) < 0) {
		printf("expire key: %s error: %s\r\n",
			key.c_str(), cmd.result_error());
		return false;
	} else if (i < 10) {
		printf("expire ok, key: %s\r\n", key.c_str());
	}

	return true;
}

static bool test_ttl(acl::redis_key& cmd, int i)
{
	cmd.clear();

	acl::string key;
	int ttl;

	key.format("%s_%d", __keypre.c_str(), i);
	if ((ttl = cmd.ttl(key.c_str())) < 0) {
		printf("get ttl key: %s error: %s\r\n",
			key.c_str(), cmd.result_error());
		return false;
	} else if (i < 10) {
		printf("ttl ok, key: %s, ttl: %d\r\n", key.c_str(), ttl);
	}
	return true;
}

static bool test_exists(acl::redis_key& cmd, int i)
{
	cmd.clear();

	acl::string key;

	key.format("%s_%d", __keypre.c_str(), i);
	if (!cmd.exists(key.c_str())) {
		if (i < 10) {
			printf("no exists key: %s\r\n", key.c_str());
		}
	} else {
		if (i < 10) {
			printf("exists key: %s\r\n", key.c_str());
		}
	}
	return true;
}

static bool test_type(acl::redis_key& cmd, int i)
{
	cmd.clear();

	acl::string key;

	key.format("%s_%d", __keypre.c_str(), i);
	acl::redis_key_t ret = cmd.type(key.c_str());
	if (ret == acl::REDIS_KEY_NONE) {
		printf("unknown type key: %s\r\n", key.c_str());
		return false;
	} else if (i < 10) {
		printf("type ok, key: %s, ret: %d\r\n", key.c_str(), ret);
	}
	return true;
}

static bool test_set(acl::redis_string& cmd, int i)
{
	cmd.clear();

	acl::string key;
	key.format("%s_%d", __keypre.c_str(), i);

	acl::string value;
	value.format("value_%s", key.c_str());

	bool ret = cmd.set(key.c_str(), value.c_str());
	return ret;
	if (i < 10) {
		printf("set key: %s, value: %s %s\r\n", key.c_str(),
			value.c_str(), ret ? "ok" : "error");
	}
	return ret;
}

static bool test_get(acl::redis_string& cmd, int i)
{
	cmd.clear();

	acl::string key;
	key.format("%s_%d", __keypre.c_str(), i);

	acl::string value;

	bool ret = cmd.get(key.c_str(), value);
	if (i < 10) {
		printf("get key: %s, value: %s %s, len: %d\r\n",
			key.c_str(), value.c_str(), ret ? "ok" : "error",
			(int) value.length());
	}
	return ret;
}

static int __threads_exit = 0;

class test_thread : public acl::thread
{
public:
	test_thread(acl::locker& locker, acl::redis_client_pipeline& conns,
		const char* cmd, int n)
	: locker_(locker)
	, conns_(conns)
	, cmd_(cmd)
	, n_(n)
	{}

	~test_thread(void) {}

protected:
	// @override
	void* run(void)
	{
		bool ret;
		acl::redis_key cmd_key;
		cmd_key.set_pipeline(&conns_);

		acl::redis_string cmd_string;
		cmd_string.set_pipeline(&conns_);

		for (int i = 0; i < n_; i++) {
			if (cmd_ == "set") {
				ret = test_set(cmd_string, i);
			} else if (cmd_ == "get") {
				ret = test_get(cmd_string, i);
			} else if (cmd_ == "del") {
				ret = test_del(cmd_key, i);
			} else if (cmd_ == "expire") {
				ret = test_expire(cmd_key, i);
			} else if (cmd_ == "ttl") {
				ret = test_ttl(cmd_key, i);
			} else if (cmd_ == "exists") {
				ret = test_exists(cmd_key, i);
			} else if (cmd_ == "type") {
				ret = test_type(cmd_key, i);
			} else if (cmd_ == "all") {
				if (!test_set(cmd_string, i)
				    || !test_get(cmd_string, i)
				    || !test_exists(cmd_key, i)
				    || !test_type(cmd_key, i)
				    || !test_expire(cmd_key, i)
				    || !test_ttl(cmd_key, i)
				    || !test_del(cmd_key, i)) {
					ret = false;
				} else {
					ret = true;
				}
			} else {
				printf("unknown cmd: %s\r\n", cmd_.c_str());
				break;
			}

			if (!ret) {
				printf("cmd: %s error, tid: %lu\r\n",
					cmd_.c_str(), thread_self());
				break;
			}

			/*
			if (i > 0 && i % 1000 == 0) {
				char tmp[128];
				acl::safe_snprintf(tmp, sizeof(tmp), "%d", i);
				acl::meter_time(__FILE__, __LINE__, tmp);
			}
			*/

			cmd_string.clear();
			cmd_key.clear();
		}

		locker_.lock();
		__threads_exit++;
		locker_.unlock();

		return NULL;
	}

private:
	acl::locker& locker_;
	acl::redis_client_pipeline& conns_;
	acl::string cmd_;
	int n_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s one_redis_addr[127.0.0.1:6379]\r\n"
		"-n count[default: 10]\r\n"
		"-t max_threads[default: 10]\r\n"
		"-w wait_for_cluster_resume[default: 500 ms]\r\n"
		"-r retry_for_cluster_resnum[default: 10]\r\n"
		"-p password [set the password of redis cluster]\r\n"
		"-a cmd[set|get|expire|ttl|exists|type|del]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 1;
	int  max_threads = 10;
	acl::string addr("127.0.0.1:6379"), cmd("del"), passwd;

	while ((ch = getopt(argc, argv, "hs:n:t:a:p:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'n':
			n = atoi(optarg);
			break;
		case 't':
			max_threads = atoi(optarg);
			break;
		case 'a':
			cmd = optarg;
			break;
		case 'p':
			passwd = optarg;
			break;;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl::redis_client_pipeline pipeline(addr);
	if (!passwd.empty()) {
		pipeline.set_password(passwd);
	}
	pipeline.start_thread();

	struct timeval begin;
	gettimeofday(&begin, NULL);

	acl::locker locker;

	std::vector<test_thread*> threads;
	for (int i = 0; i < max_threads; i++) {
		test_thread* thread = new test_thread(locker, pipeline,
			cmd.c_str(), n);
		threads.push_back(thread);
		thread->set_detachable(true);
		thread->start();
	}

	while (true) {
		locker.lock();
		if (__threads_exit == max_threads) {
			locker.unlock();
			printf("All threads over now!\r\n");
			break;
		}
		locker.unlock();

		//printf("max_threads: %d, threads_exit: %d\r\n",
		//	max_threads, __threads_exit);
		sleep(1);
	}

	std::vector<test_thread*>::iterator it = threads.begin();
	for (; it != threads.end(); ++it) {
		//(*it)->wait();
		delete (*it);
	}

	struct timeval end;
	gettimeofday(&end, NULL);

	long long int total = max_threads * n;
	double inter = util::stamp_sub(&end, &begin);
	printf("total %s: %lld, spent: %0.2f ms, speed: %0.2f\r\n", cmd.c_str(),
		total, inter, (total * 1000) /(inter > 0 ? inter : 1));

	pipeline.stop_thread();

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
