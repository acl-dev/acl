#include "stdafx.h"

static bool test_multi(acl::redis_transaction& redis)
{
	redis.clear();
	if (redis.multi() == false)
	{
		printf("multi error\r\n");
		return false;
	}
	printf("multi ok\r\n");
	return true;
}

static bool test_run_cmds(acl::redis_transaction& redis)
{
	std::vector<acl::string> args;
	const char* cmd = "SET";

	args.push_back("multi_string_key");
	args.push_back("multi_string_value");

	if (redis.run_cmd(cmd, args) == false)
	{
		printf("run cmd: %s error\r\n", cmd);
		return false;
	}

	args.clear();
	redis.clear();

	cmd = "HMSET";
	args.push_back("multi_hash_key");
	args.push_back("name1");
	args.push_back("value1");
	args.push_back("name2");
	args.push_back("values");

	if (redis.run_cmd(cmd, args) == false)
	{
		printf("run cmd: %s error\r\n", cmd);
		return false;
	}

	args.clear();
	redis.clear();

	cmd = "GET";
	args.push_back("multi_string_key");

	if (redis.run_cmd(cmd, args) == false)
	{
		printf("run cmd: %s error\r\n", cmd);
		return false;
	}

	args.clear();
	redis.clear();

	cmd = "HGET";
	args.push_back("multi_hash_key");
	args.push_back("name1");

	if (redis.run_cmd(cmd, args) == false)
	{
		printf("run cmd: %s error\r\n", cmd);
		return false;
	}

	printf("run all cmds ok\r\n");
	return true;
}

static bool test_exec(acl::redis_transaction& redis)
{
	redis.clear();
	if (redis.exec() == false)
	{
		printf("exec error\r\n");
		return false;
	}
	printf("exec ok\r\n");
	return true;
}

static bool get_results(acl::redis_transaction& redis)
{
	const std::vector<acl::string>& cmds = redis.get_commands();
	size_t size = redis.get_size();

	if (size != cmds.size())
	{
		printf("invalid size: %lu != %lu\r\n",
			(unsigned long) size, (unsigned long) cmds.size());
		return false;
	}

	/////////////////////////////////////////////////////////////////////
	// cmd: SET

	const acl::redis_result* result = redis.get_child(0, NULL);
	if (result == NULL)
	{
		printf("have no result for cmd %s\r\n", cmds[0].c_str());
		return false;
	}
	if (result->get_type() != acl::REDIS_RESULT_STATUS)
	{
		printf("invalid result for cmd %s\r\n", cmds[0].c_str());
		return false;
	}
	else
		printf("run cmd %s ok\r\n", cmds[0].c_str());

	/////////////////////////////////////////////////////////////////////
	// cmd: HMSET

	result = redis.get_child(1, NULL);
	if (result == NULL)
	{
		printf("have no result for cmd %s\r\n", cmds[0].c_str());
		return false;
	}
	if (result->get_type() != acl::REDIS_RESULT_STATUS)
	{
		printf("invalid result for cmd %s\r\n", cmds[0].c_str());
		return false;
	}
	else
		printf("run cmd %s ok\r\n", cmds[0].c_str());

	/////////////////////////////////////////////////////////////////////
	// cmd: GET

	result = redis.get_child(2, NULL);
	if (result == NULL)
	{
		printf("have no result for cmd %s\r\n", cmds[0].c_str());
		return false;
	}
	if (result->get_type() != acl::REDIS_RESULT_STRING)
	{
		printf("invalid result for cmd %s\r\n", cmds[0].c_str());
		return false;
	}
	else
		printf("run cmd %s ok, result: %s\r\n", cmds[0].c_str(),
			result->get(0));

	/////////////////////////////////////////////////////////////////////
	// cmd: HGET

	result = redis.get_child(3, NULL);
	if (result == NULL)
	{
		printf("have no result for cmd %s\r\n", cmds[0].c_str());
		return false;
	}
	if (result->get_type() != acl::REDIS_RESULT_STRING)
	{
		printf("invalid result for cmd %s\r\n", cmds[0].c_str());
		return false;
	}
	else
		printf("run cmd %s ok, result: %s\r\n", cmds[0].c_str(),
			result->get(0));

	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s redis_addr[127.0.0.1:6379]\r\n"
		"-C connect_timeout[default: 10]\r\n"
		"-I rw_timeout[default: 10]\r\n"
		"-S [if slice request, default: no]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, conn_timeout = 10, rw_timeout = 10;
	acl::string addr("127.0.0.1:6379");
	bool slice_req = false;

	while ((ch = getopt(argc, argv, "hs:C:I:S")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'C':
			conn_timeout = atoi(optarg);
			break;
		case 'I':
			rw_timeout = atoi(optarg);
			break;
		case 'S':
			slice_req = true;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::redis_client client(addr.c_str(), conn_timeout, rw_timeout);
	client.set_slice_request(slice_req);
	client.set_slice_respond(false);
	acl::redis_transaction redis(&client);

	bool ret;

	ret = test_multi(redis) && test_run_cmds(redis)
		&& test_exec(redis) && get_results(redis);

	printf("all cmds %s\r\n", ret ? "ok" : "failed");

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
