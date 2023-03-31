#include "stdafx.h"

static acl::string __keypre("test_key_cluster");

static bool test_del(acl::redis_key& cmd, int i)
{
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

static bool test_hmset(acl::redis_hash& cmd, int i)
{
	acl::string key;
	key.format("hash-%s-%d", __keypre.c_str(), i);

	std::map<acl::string, acl::string> attrs;
	attrs["name1"] = "value1";
	attrs["name2"] = "value2";
	attrs["name3"] = "value3";

	if (!cmd.hmset(key, attrs)) {
		printf("hmset %s error=%s\r\n", key.c_str(), cmd.result_error());
		return false;
	}
	if (i < 10) {
		printf("hmset %s ok\r\n", key.c_str());
	}
	return true;
}

static bool test_hmget(acl::redis_hash& cmd, int i)
{
	acl::string key;
	key.format("hash-%s-%d", __keypre.c_str(), i);

	std::vector<acl::string> names;
	names.push_back("name1");
	names.push_back("name2");
	names.push_back("name3");

	std::vector<acl::string> values;

	if (!cmd.hmget(key, names,&values)) {
		printf("hmget error %s\r\n", cmd.result_error());
		return false;
	}

	if (i < 10) {
		printf("key=%s:", key.c_str());
		assert(names.size() == values.size());
		size_t n = names.size();
		for (size_t j = 0; j < n; j++) {
			printf(" %s->%s", names[j].c_str(), values[j].c_str());
		}
		printf("\r\n");
	}

	return true;
}

static int __threads_exit = 0;

class test_fiber : public acl::fiber
{
public:
	test_fiber(acl::redis_client_pipeline& conns, const char* cmd, int n)
	: conns_(conns)
	, cmd_(cmd)
	, n_(n)
	{}

	~test_fiber(void) {}

protected:
	// @override
	void run(void)
	{
		bool ret;
		acl::redis redis;
		redis.set_pipeline(&conns_);

		for (int i = 0; i < n_; i++) {
			if (cmd_ == "set") {
				ret = test_set(redis, i);
			} else if (cmd_ == "get") {
				ret = test_get(redis, i);
			} else if (cmd_ == "del") {
				ret = test_del(redis, i);
			} else if (cmd_ == "expire") {
				ret = test_expire(redis, i);
			} else if (cmd_ == "ttl") {
				ret = test_ttl(redis, i);
			} else if (cmd_ == "exists") {
				ret = test_exists(redis, i);
			} else if (cmd_ == "type") {
				ret = test_type(redis, i);
			} else if (cmd_ == "all") {
				if (!test_set(redis, i)
				    || !test_get(redis, i)
				    || !test_exists(redis, i)
				    || !test_type(redis, i)
				    || !test_expire(redis, i)
				    || !test_ttl(redis, i)
				    || !test_del(redis, i)) {
					ret = false;
				} else {
					ret = true;
				}
			} else if (cmd_ == "hmset") {
				ret = test_hmset(redis, i);
			} else if (cmd_ == "hmget") {
				ret = test_hmget(redis, i);
			} else {
				printf("unknown cmd: %s\r\n", cmd_.c_str());
				break;
			}

			if (!ret) {
				printf("cmd: %s error, tid: %lu\r\n",
					cmd_.c_str(), acl::thread::self());
				break;
			}

			/*
			if (i > 0 && i % 50000 == 0) {
				char tmp[128];
				acl::safe_snprintf(tmp, sizeof(tmp), "%d", i);
				acl::meter_time(__FILE__, __LINE__, tmp);
			}
			*/

			redis.clear();
		}
	}

private:
	acl::redis_client_pipeline& conns_;
	acl::string cmd_;
	//acl::redis redis;
	int n_;
};

class test_thread : public acl::thread
{
public:
	test_thread(acl::locker& locker, acl::redis_client_pipeline& conns,
		const char* cmd, int n, size_t nfibers, size_t stack_size,
		bool share_stack)
	: locker_(locker)
	, conns_(conns)
	, cmd_(cmd)
	, n_(n)
	, nfibers_(nfibers)
	, stack_size_(stack_size)
	, share_stack_(share_stack)
	{}

	~test_thread(void) {}

protected:
	void* run(void)
	{
		std::vector<acl::fiber*> fibers;
		for (size_t i = 0; i < nfibers_; i++) {
			test_fiber* fb = new test_fiber(conns_, cmd_, n_);
			fibers.push_back(fb);
			fb->start(stack_size_, share_stack_);
		}

		acl::fiber::schedule();

		for (std::vector<acl::fiber*>::iterator it = fibers.begin();
			it != fibers.end(); ++it) {
			delete *it;
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
	size_t nfibers_;
	size_t stack_size_;
	bool   share_stack_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s one_redis_addr[127.0.0.1:6379]\r\n"
		"-F [if using fiber_redis_pipeline, default: false]\r\n"
		"-n count[default: 10]\r\n"
		"-C connect_timeout[default: 10]\r\n"
		"-I rw_timeout[default: 10]\r\n"
		"-t max_threads[default: 10]\r\n"
		"-c fibers_count[default: 50]]\r\n"
		"-S [if use shared stack, default: false]\r\n"
		"-z stack_size[default: 64000]\r\n"
		"-w wait_for_cluster_resume[default: 500 ms]\r\n"
		"-r retry_for_cluster_resnum[default: 10]\r\n"
		"-p password [set the password of redis cluster]\r\n"
		"-a cmd[set|get|hmset|hmget|expire|ttl|exists|type|del]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 1, conn_timeout = 10, rw_timeout = 10;
	int  max_threads = 10, nfibers = 50;
	size_t stack_size = 64000;
	bool share_stack = false, use_fiber_tbox = false;
	acl::string addr("127.0.0.1:6379"), cmd, passwd;

	while ((ch = getopt(argc, argv, "hs:n:C:I:t:c:a:p:Sz:F")) > 0) {
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
		case 'C':
			conn_timeout = atoi(optarg);
			break;
		case 'I':
			rw_timeout = atoi(optarg);
			break;
		case 't':
			max_threads = atoi(optarg);
			break;
		case 'c':
			nfibers = atoi(optarg);
			break;
		case 'a':
			cmd = optarg;
			break;
		case 'p':
			passwd = optarg;
			break;;
		case 'S':
			share_stack = true;
			break;
		case 'z':
			stack_size = (size_t) atoi(optarg);
			break;
		case 'F':
			use_fiber_tbox = true;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl::redis_client_pipeline* pipeline;
	if (use_fiber_tbox) {
		pipeline = new acl::fiber_redis_pipeline(addr);
	} else {
		pipeline = new acl::redis_client_pipeline(addr);
	}

	pipeline->set_timeout(conn_timeout, rw_timeout);
	if (!passwd.empty()) {
		pipeline->set_password(passwd);
	}

	// start pipeline thread to handle all redis command messages
	pipeline->start_thread();

	struct timeval begin;
	gettimeofday(&begin, NULL);

	acl::locker locker;

	std::vector<test_thread*> threads;
	for (int i = 0; i < max_threads; i++) {
		test_thread* thread = new test_thread(locker, *pipeline,
			cmd.c_str(), n, nfibers, stack_size, share_stack);
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

	long long int total = max_threads * n * nfibers;
	struct timeval end;
	gettimeofday(&end, NULL);

	double inter = acl::stamp_sub(end, begin);
	printf("total %s: %lld, spent: %0.2f ms, speed: %0.2f\r\n", cmd.c_str(),
		total, inter, (total * 1000) /(inter > 0 ? inter : 1));

	std::vector<test_thread*>::iterator it = threads.begin();
	for (; it != threads.end(); ++it) {
		// don't wait for the thread to exit because it has beeen
		// detached when started.
		delete (*it);
	}

	// stop pipeline thread and delete pipeline object.
	pipeline->stop_thread();
	delete pipeline;

	printf("The pipeline thread has stopped!\r\n");

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
