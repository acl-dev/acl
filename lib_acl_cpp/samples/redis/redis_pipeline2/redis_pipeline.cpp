#include "stdafx.h"
#include "util.h"

static int __threads_exit = 0;

class redis_command {
public:
	redis_command(acl::redis_client_pipeline& pipeline)
	: msg_(cmd_.get_pipeline_message())
	{
		cmd_.set_pipeline(&pipeline);
		argc_ = 2;
		argv_[0] = "del";
		argv_[1] = "test-key";
		lens_[0] = strlen(argv_[0]);
		lens_[1] = strlen(argv_[1]);
		cmd_.build_request(argc_, argv_, lens_);
	}

	~redis_command(void) {}

	acl::redis_pipeline_message& get_message(void) {
		return msg_;
	}

private:
	acl::redis cmd_;
	acl::redis_pipeline_message& msg_;
	size_t argc_;
	const char* argv_[2];
	size_t lens_[2];

};

class test_thread : public acl::thread {
public:
	test_thread(acl::locker& locker, acl::redis_client_pipeline& pipeline,
		const char* cmd, int n)
	: locker_(locker)
	, pipeline_(pipeline)
	, cmd_(cmd)
	, n_(n)
	{
		(void) cmd_;
	}

	~test_thread(void) {}

protected:
	// @override
	void* run(void) {
		std::vector<redis_command*> commands;
		for (size_t i = 0; i < 10; i++) {
			redis_command* command = new redis_command(pipeline_);
			commands.push_back(command);
		}

		for (int i = 0; i < n_; i++) {
			request(commands);
		}

		for (std::vector<redis_command*>::iterator it = commands.begin();
			    it != commands.end(); ++it) {
			delete *it;
		}

		locker_.lock();
		__threads_exit++;
		locker_.unlock();

		return NULL;
	}

private:
	void request(std::vector<redis_command*>& commands) {
		// send all request commands
		for (std::vector<redis_command*>::iterator it = commands.begin();
			it != commands.end(); ++it) {
			acl::redis_pipeline_message& msg =
				(*it)->get_message();
			pipeline_.push(&msg);
		}

		// wait for all results
		for (std::vector<redis_command*>::iterator it = commands.begin();
			it != commands.end(); ++it) {
			acl::redis_pipeline_message& msg =
				(*it)->get_message();
			const acl::redis_result* result = msg.wait();
			if (result == NULL) {
				printf("wait result error\r\n");
				break;
			}
		}
	}

private:
	acl::locker& locker_;
	acl::redis_client_pipeline& pipeline_;
	acl::string cmd_;
	int n_;
};

static void usage(const char* procname) {
	printf("usage: %s -h[help]\r\n"
		"-s one_redis_addr[127.0.0.1:6379]\r\n"
		"-n count[default: 10]\r\n"
		"-C connect_timeout[default: 10]\r\n"
		"-I rw_timeout[default: 10]\r\n"
		"-c max_threads[default: 10]\r\n"
		"-w wait_for_cluster_resume[default: 500 ms]\r\n"
		"-r retry_for_cluster_resnum[default: 10]\r\n"
		"-P password [set the password of redis cluster]\r\n"
		"-a cmd[set|get|expire|ttl|exists|type|del]\r\n",
		procname);
}

int main(int argc, char* argv[]) {
	int  ch, n = 1, conn_timeout = 10, rw_timeout = 10;
	int  max_threads = 10;
	acl::string addr("127.0.0.1:6379"), cmd("del"), passwd;

	while ((ch = getopt(argc, argv, "hs:n:C:I:c:a:p:")) > 0) {
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
		case 'c':
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

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
