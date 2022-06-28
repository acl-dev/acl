#include "stdafx.h"

static void show_master(const acl::redis_master& master)
{
	printf("name=%s\r\n", master.name_.c_str());
	printf("ip=%s\r\n", master.ip_.c_str());
	printf("port=%d\r\n", master.port_);
	printf("runid=%s\r\n", master.runid_.c_str());
	printf("flags=%s\r\n", master.flags_.c_str());
}

static void sentinel_master(acl::redis_sentinel& sentinel, const char* master_name)
{
	acl::redis_master master;
	if (sentinel.sentinel_master(master_name, master) == false) {
		printf("sentinel_master error, master=%s\r\n", master_name);
		return;
	}

	show_master(master);
}

static void sentinel_masters(acl::redis_sentinel& sentinel)
{
	std::vector<acl::redis_master> masters;
	if (sentinel.sentinel_masters(masters) == false) {
		printf("sentinel_masters error\r\n");
		return;
	}

	for (std::vector<acl::redis_master>::const_iterator
		cit = masters.begin(); cit != masters.end(); ++cit) {
		if (cit != masters.begin())
			printf("-----------------------------------\r\n");
		show_master(*cit);
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
	       "-s sentinel_addr[127.0.0.1:6379]\r\n"
	       "-C connect_timeout[default: 10]\r\n"
	       "-T rw_timeout[default: 10]\r\n"
	       "-a cmd[master|masters]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int ch, conn_timeout = 10, rw_timeout = 10;
	acl::string addr("127.0.0.1:26379"), cmd("master");
	acl::string master_name("mymaster");

	while ((ch = getopt(argc, argv, "hs:n:C:T:a:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'C':
			conn_timeout = atoi(optarg);
			break;
		case 'T':
			rw_timeout = atoi(optarg);
			break;
		case 'a':
			cmd = optarg;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::redis_client client(addr.c_str(), conn_timeout, rw_timeout);
	acl::redis_sentinel sentinel(&client);

	if (cmd == "master")
		sentinel_master(sentinel, master_name);
	else if (cmd == "masters")
		sentinel_masters(sentinel);
	else
		printf("unknown cmd=%s\r\n", cmd.c_str());

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
