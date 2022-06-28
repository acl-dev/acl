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

static bool test_getjob(acl::disque& cmd, bool ack, int i)
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
	
	if (ack)
	{
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
	}

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

static bool test_enqueue(acl::disque& cmd,
	const std::vector<acl::string>& job_ids, int i)
{
	cmd.clear();
	int ret = cmd.enqueue(job_ids);
	if (ret < 0)
		printf("enqueue error: %s\r\n", cmd.result_error());
	else if (i < 10)
		printf("enqueue ok\r\n");

	return true;
}

static bool test_dequeue(acl::disque& cmd,
	const std::vector<acl::string>& job_ids, int i)
{
	cmd.clear();
	int ret = cmd.dequeue(job_ids);
	if (ret < 0)
		printf("dequeue error: %s\r\n", cmd.result_error());
	else if (i < 10)
		printf("dequeue ok\r\n");

	return true;
}

static bool test_deljob(acl::disque& cmd,
	const std::vector<acl::string>& job_ids, int i)
{
	cmd.clear();
	int ret = cmd.deljob(job_ids);
	if (ret < 0)
		printf("deljob error: %s\r\n", cmd.result_error());
	else if (i < 10)
		printf("deljob ok\r\n");

	return true;
}

static bool test_info(acl::disque& cmd, int i)
{
	cmd.clear();
	std::map<acl::string, acl::string> infos;
	if (cmd.info(infos) == false)
	{
		printf("info error: %s\r\n", cmd.result_error());
		return false;
	}
	else if (i >= 10)
		return true;

	std::map<acl::string, acl::string>::const_iterator cit;
	for (cit = infos.begin(); cit != infos.end(); ++cit)
		printf("%s: %s\r\n", cit->first.c_str(), cit->second.c_str());
	return true;
}

static bool test_hello(acl::disque& cmd, int i)
{
	cmd.clear();
	const std::vector<acl::disque_node*>* nodes = cmd.hello();
	if (nodes == NULL)
	{
		printf("hello error: %s\r\n", cmd.result_error());
		return false;
	}
	else if (i >= 10)
		return true;

	std::vector<acl::disque_node*>::const_iterator cit;
	for (cit = nodes->begin(); cit != nodes->end(); ++cit)
	{
		printf("id: %s, ip: %s, port: %d\r\n", (*cit)->get_id(),
			(*cit)->get_ip(), (*cit)->get_port());
	}
	return true;
}

static bool test_show(acl::disque& cmd, const char* jobid, int i)
{
	acl::string key;

	key.format("%s_%d", __jobpre.c_str(), i);
	cmd.clear();
	const acl::disque_job* job = cmd.show(jobid);
	if (job == NULL)
	{
		printf("show jobid: %s error: %s\r\n",
			jobid, cmd.result_error());
		return false;
	}
	else if (i >= 10)
		return true;

	printf("show ok, jobid: %s\r\n", jobid);
	printf("id: %s\r\n", job->get_id());
	printf("queue: %s\r\n", job->get_queue());
	printf("state: %s\r\n", job->get_state());
	printf("repl: %d\r\n", job->get_repl());
	printf("ttl: %d\r\n", job->get_ttl());
	printf("ctime: %lld\r\n", job->get_ctime());
	printf("delay: %d\r\n", job->get_delay());
	printf("retry: %d\r\n", job->get_repl());
	printf("next_requeue_within: %d\r\n", job->get_next_requeue_within());
	printf("next_awake_within: %d\r\n", job->get_next_awake_within());
	printf("body: %s\r\n", job->get_body().c_str());
	printf(">>>nodes_delivered\r\n");
	const std::vector<acl::string>& nodes = job->get_nodes_delivered();
	for (std::vector<acl::string>::const_iterator cit = nodes.begin();
		cit != nodes.end(); ++cit)
	{
		printf("\t%s\r\n", (*cit).c_str());
	}

	printf(">>>nodes_confirmed\r\n");
	const std::vector<acl::string>& nodes2 = job->get_nodes_confirmed();
	for (std::vector<acl::string>::const_iterator cit = nodes2.begin();
		cit != nodes2.end(); ++cit)
	{
		printf("\t%s\r\n", (*cit).c_str());
	}

	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s disque_addr[127.0.0.1:7711]\r\n"
		"-n count\r\n"
		"-c connect_timeout[default: 10]\r\n"
		"-t rw_timeout[default: 10]\r\n"
		"-D delay\r\n"
		"-R replicate\r\n"
		"-r retry\r\n"
		"-T ttl\r\n"
		"-M maxlen\r\n"
		"-A [async]\r\n"
		"-I jobid\r\n"
		"-C [if need ackjob for getjob]\r\n"
		"-a cmd[addjob|getjob|qlen|qpeek|show|ackjob|fastack|enqueue|dequeue|deljob|info|hello]\r\n",
		procname);

	printf("sample:\r\n"
		"%s -s 127.0.0.1:7711 -n 10000 -D 1 -R 2 -r 1 -T 1000 -M 1000000 -a addjob\r\n"
		"%s -s 127.0.0.1:7711 -n 10000 -a getjob\r\n"
		"%s -s 127.0.0.1:7711 -n 10000 -a getjob -C\r\n"
		"%s -s 127.0.0.1:7711 -n 10 -a qlen\r\n"
		"%s -s 127.0.0.1:7711 -n 100 -a qpeek\r\n"
		"%s -s 127.0.0.1:7711 -n 1 -a info\r\n"
		"%s -s 127.0.0.1:7711 -n 1 -a hello\r\n",
		procname, procname, procname, procname, procname, procname, procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 1, conn_timeout = 10, rw_timeout = 10;
	acl::string addr("127.0.0.1:7711"), command, jobid;
	std::vector<acl::string> job_ids;
	acl::disque_cond cond;
	bool ack = false;

	while ((ch = getopt(argc, argv, "hs:n:c:t:a:D:R:r:T:M:AI:C")) > 0)
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
		case 'c':
			conn_timeout = atoi(optarg);
			break;
		case 't':
			rw_timeout = atoi(optarg);
			break;
		case 'a':
			command = optarg;
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
		case 'I':
			jobid = optarg;
			job_ids.push_back(jobid);
			break;
		case 'C':
			ack = true;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);
	acl::disque_client client(addr.c_str(), conn_timeout, rw_timeout);
	acl::disque cmd(&client);

	bool ret;

	for (int i = 0; i < n; i++)
	{
		if (command == "addjob")
			ret = test_addjob(cmd, cond, i);
		else if (command == "getjob")
			ret = test_getjob(cmd, ack, i);
		else if (command == "qlen")
			ret = test_qlen(cmd, i);
		else if (command == "qpeek")
			ret = test_qpeek(cmd, i);
		else if (command == "show")
		{
			if (jobid.empty())
			{
				printf("need: -I jobid\r\n");
				break;
			}
			ret = test_show(cmd, jobid.c_str(), i);
		}
		else if (command == "ackjob")
		{
			if (job_ids.empty())
			{
				printf("need: -I jobid\r\n");
				break;
			}
			ret = test_ackjob(cmd, job_ids, i);
		}
		else if (command == "enqueue")
		{
			if (job_ids.empty())
			{
				printf("need: -I jobid\r\n");
				break;
			}
			ret = test_enqueue(cmd, job_ids, i);
		}
		else if (command == "dequeue")
		{
			if (job_ids.empty())
			{
				printf("need: -I jobid\r\n");
				break;
			}
			ret = test_dequeue(cmd, job_ids, i);
		}
		else if (command == "deljob")
		{
			if (job_ids.empty())
			{
				printf("need: -I jobid\r\n");
				break;
			}
			ret = test_deljob(cmd, job_ids, i);
		}
		else if (command == "info")
			ret = test_info(cmd, i);
		else if (command == "hello")
			ret = test_hello(cmd, i);
		else
		{
			printf("unknown cmd: %s\r\n", command.c_str());
			break;
		}

		if (ret == false)
		{
			printf("test failed!\r\n");
			break;
		}
	}

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
