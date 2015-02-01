#include "stdafx.h"

static const char* __keypre = "hyperloglog_";

static bool test_pfadd(acl::redis_hyperloglog& option, int n)
{
	acl::string key, element;
	std::vector<acl::string> elements;

	elements.reserve(1000);
	for (int i = 0; i < 1000; i++)
	{
		element.format("element_%d", i);
		elements.push_back(element);
	}

	for (int i = 0; i < n; i++)
	{
		key.format("%s_key_%d", __keypre, i);

		option.reset();
		int ret = option.pfadd(key.c_str(), elements);
		if (ret < 0)
		{
			printf("pfadd error, key: %s\r\n", key.c_str());
			return false;
		}

		if (i >= 10)
			continue;

		printf("pfadd ok\r\n");
	}

	return true;
}

static bool test_pfcount(acl::redis_hyperloglog& option, int n)
{
	acl::string key;
	std::vector<acl::string> keys;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_key_%d", __keypre, i);
		keys.push_back(key);
	}

	option.reset();
	int ret = option.pfcount(keys);
	if (ret < 0)
	{
		printf("pfcount error\r\n");
		return false;
	}

	printf("pfcount ok: %d\r\n", ret);
	return true;
}

static bool test_pfmerge(acl::redis_hyperloglog& option, int n)
{
	acl::string key;
	std::vector<acl::string> keys;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_key_%d", __keypre, i);
		keys.push_back(key);
	}

	acl::string dest_key;
	dest_key.format("%s_dest_key", __keypre);
	option.reset();
	if (option.pfmerge(dest_key, keys) == false)
	{
		printf("pfmerge failed, dest: %s\r\n", dest_key.c_str());
		return false;
	}

	printf("pfmerge ok, dest: %s\r\n", dest_key.c_str());

	option.reset();
	int ret = option.pfcount(dest_key.c_str(), NULL);
	printf("pfcont ok, count: %d, key: %s\r\n", ret, dest_key.c_str());

	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s redis_addr[127.0.0.1:6379]\r\n"
		"-n count\r\n"
		"-C connect_timeout[default: 10]\r\n"
		"-I rw_timeout[default: 10]\r\n"
		"-S [if slice request, default: no]\r\n"
		"-a cmd[pfadd|pfcount|pfmerge]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 1, conn_timeout = 10, rw_timeout = 10;
	acl::string addr("127.0.0.1:6379"), cmd;
	bool slice_req = false;

	while ((ch = getopt(argc, argv, "hs:n:C:I:a:S")) > 0)
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
		case 'a':
			cmd = optarg;
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
	acl::redis_hyperloglog option(&client);

	bool ret;

	if (cmd == "pfadd")
		ret = test_pfadd(option, n);
	else if (cmd == "pfcount")
		ret = test_pfcount(option, n);
	else if (cmd == "pfmerge")
		ret = test_pfmerge(option, n);
	else if (cmd == "all")
	{
		ret = test_pfadd(option, n)
			&& test_pfcount(option, n)
			&& test_pfmerge(option, n);
	}
	else
	{
		printf("unknown cmd: %s\r\n", cmd.c_str());
		ret = false;
	}

	printf("cmd: %s %s\r\n", cmd.c_str(), ret ? "ok" : "failed");

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
