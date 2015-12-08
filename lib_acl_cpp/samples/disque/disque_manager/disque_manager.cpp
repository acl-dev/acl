#include "stdafx.h"

static acl::string __queue("greeting");
static acl::string __jobpre("test_job");

static bool test_addjob(acl::disque& cmd, const acl::disque_cond& cond, int i)
{
	acl::string job;
	int timeout = 0;
	const char* jobid;

	job.format("job_%s_%d", __jobpre.c_str(), i);
	cmd.clear();
	jobid = cmd.addjob(__queue, job, timeout, &cond);
	if (jobid == NULL)
	{
		printf("addjob queue: %s error: %s\r\n",
			__queue.c_str(), cmd.result_error());
		return false;
	}
	else if (i < 10)
		printf("addjob queue: %s ok, jobid: %s\r\n",
			__queue.c_str(), jobid);

	return true;
}

static bool test_ackjob(acl::disque& cmd,
	const std::vector<acl::string>& job_ids, int i)
{
	cmd.clear();
	int ret = cmd.ackjob(job_ids);
	if (ret < 0)
		printf("ackjob error: %s\r\n", cmd.result_error());
	else if (i < 10)
		printf("ackjob ok, ret: %d\r\n", ret);

	return true;
}

static bool test_getjob(acl::disque& cmd, int i)
{
	std::vector<acl::string> queues;
	size_t timeout = 10, count = 10;

	queues.push_back(__queue);

	cmd.clear();
	const std::vector<acl::disque_job*>* jobs =
		cmd.getjob(queues, timeout, count);
	if (jobs == NULL)
	{
		printf("getjob queue: %s error: %s\r\n",
			__queue.c_str(), cmd.result_error());
		return false;
	}

	std::vector<acl::string> job_ids;
	std::vector<acl::disque_job*>::const_iterator cit1;
	for (cit1 = jobs->begin(); cit1 != jobs->end(); ++cit1)
	{
		const char* jobid = (*cit1)->get_id();
		if (*jobid)
			job_ids.push_back(jobid);
	}

	if (!job_ids.empty() && !test_ackjob(cmd, job_ids, i))
		return false;

	if (i >= 10)
		return true;

	printf(">>getjob ok\r\n");
	std::vector<acl::disque_job*>::const_iterator cit2;
	for (cit2 = jobs->begin(); cit2 != jobs->end(); ++cit2)
	{
		printf("\tid: %s\r\n", (*cit2)->get_id());
		printf("\tqueue: %s\r\n", (*cit2)->get_queue());
		printf("\tjob: %s\r\n", (*cit2)->get_body().c_str());
	}

	return true;
}

static bool test_qlen(acl::disque& cmd, int i)
{
	cmd.clear();
	int ret = cmd.qlen(__queue.c_str());
	if (ret < 0)
	{
		printf("qlen queue: %s error: %s\r\n",
			__queue.c_str(), cmd.result_error());
		return false;
	}
	else if (i < 10)
		printf("qlen: %d, queue: %s\r\n", ret, __queue.c_str());

	return true;
}

static bool test_qpeek(acl::disque& cmd, int i)
{
	int count = 1;

	cmd.clear();
	const std::vector<acl::disque_job*>* jobs =
		cmd.qpeek(__queue.c_str(), count);
	if (jobs == NULL)
	{
		printf("qpeek queue: %s error: %s\r\n",
			__queue.c_str(), cmd.result_error());
		return false;
	}
	else if (i >= 10)
		return true;

	printf("qpeek queue: %s ok\r\n", __queue.c_str());
	std::vector<acl::disque_job*>::const_iterator cit;
	for (cit = jobs->begin(); cit != jobs->end(); ++cit)
	{
		printf("\tid: %s\r\n", (*cit)->get_id());
		printf("\tqueue: %s\r\n", (*cit)->get_queue());
		printf("\tjob: %s\r\n", (*cit)->get_body().c_str());
	}

	return true;
}

class test_thread : public acl::thread
{
public:
	test_thread(acl::disque_client_cluster& manager,
		acl::disque_cond& cond, const char* cmd, int n)
		: manager_(manager), cond_(cond), cmd_(cmd), n_(n) {}

	~test_thread() {}

protected:
	virtual void* run()
	{
		bool ret;
		acl::disque_client_pool* pool;
		acl::disque_client* conn;
		acl::disque cmd;

		for (int i = 0; i < n_; i++)
		{
			pool = (acl::disque_client_pool*) manager_.peek();
			if (pool == NULL)
			{
				printf("peek connection pool failed\r\n");
				break;
			}

			conn = (acl::disque_client*) pool->peek();
			
			if (conn == NULL)
			{
				printf("peek disque_client failed\r\n");
				break;
			}

			cmd.set_client(conn);

			if (cmd_ == "addjob")
				ret = test_addjob(cmd, cond_, i);
			else if (cmd_ == "getjob")
				ret = test_getjob(cmd, i);
			else if (cmd_ == "qlen")
				ret = test_qlen(cmd, i);
			else if (cmd_ == "qpeek")
				ret = test_qpeek(cmd, i);
			else
			{
				printf("unknown cmd: %s\r\n", cmd_.c_str());
				break;
			}

			pool->put(conn, ret);

			if (ret == false)
				break;
		}

		return NULL;
	}

private:
	acl::disque_client_cluster& manager_;
	acl::disque_cond& cond_;
	acl::string cmd_;
	int n_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s redis_addr_list[127.0.0.1:7711, 127.0.0.1:7712, 127.0.0.1:7713]\r\n"
		"-n count[default: 10]\r\n"
		"-C connect_timeout[default: 10]\r\n"
		"-I rw_timeout[default: 10]\r\n"
		"-c max_threads[default: 10]\r\n"
		"-D delay\r\n"
		"-R replicate\r\n"
		"-r retry\r\n"
		"-T ttl\r\n"
		"-M maxlen\r\n"
		"-A [async]\r\n"
		"-a cmd[addjob|getjob|qlen|qpeek]\r\n",
		procname);

	printf("sample:\r\n"
		"%s -s \"127.0.0.1:7711, 127.0.0.1:7712, 127.0.0.1:7713\" -n 10000 -c 10 -D 1 -R 2 -M 1000000 -A -a addjob\r\n"
		"%s -s \"127.0.0.1:7711, 127.0.0.1:7712, 127.0.0.1:7713\" -n 10000 -c 10 -a getjob\r\n"
		"%s -s \"127.0.0.1:7711, 127.0.0.1:7712, 127.0.0.1:7713\" -n 1 -c 10 -a qlen\r\n"
		"%s -s \"127.0.0.1:7711, 127.0.0.1:7712, 127.0.0.1:7713\" -n 1 -c 10 -a qpeek\r\n",
		procname, procname, procname, procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 1, conn_timeout = 10, rw_timeout = 10;
	int  max_threads = 10;
	acl::disque_cond cond;
	acl::string addr_list("127.0.0.1:7711, 127.0.0.1:7712, 127.0.0.1:7713"), cmd;

	while ((ch = getopt(argc, argv, "hs:n:C:I:c:a:D:R:r:T:M:A")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr_list = optarg;
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
		case 'D':
			cond.set_delay(atoi(optarg));
			break;
		case 'R':
			cond.set_replicate(atoi(optarg));
			break;
		case 'r':
			cond.set_retry(atoi(optarg));
			break;
		case 'T':
			cond.set_ttl(atoi(optarg));
			break;
		case 'M':
			cond.set_maxlen(atoi(optarg));
			break;
		case 'A':
			cond.set_async(true);
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();

	if (addr_list.empty())
	{
		usage(argv[0]);
		return 1;
	}

	acl::disque_client_cluster manager;
	const std::vector<acl::string>& tokens = addr_list.split2(";, \t");

	std::vector<acl::string>::const_iterator cit;
	for (cit = tokens.begin(); cit != tokens.end(); ++cit)
		manager.set((*cit).c_str(), max_threads,
			conn_timeout, rw_timeout);

	std::vector<test_thread*> threads;
	for (int i = 0; i < max_threads; i++)
	{
		test_thread* thread = new test_thread(manager, cond,
			cmd.c_str(), n);
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
