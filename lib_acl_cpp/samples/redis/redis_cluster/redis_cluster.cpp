#include "stdafx.h"

static void print_slaves(const acl::redis_node* master)
{
	const std::vector<acl::redis_node*>& slaves = master->get_slaves();
	std::vector<acl::redis_node*>::const_iterator cit;
	for (cit = slaves.begin(); cit != slaves.end(); ++cit)
	{
		printf("slave: ip: %s, port: %d, slot_min: %d, slot_max: %d\r\n",
			(*cit)->get_ip(), (*cit)->get_port(),
			(int) (*cit)->get_slot_range_from(),
			(int) (*cit)->get_slot_range_to());
	}
}

static bool test_slots(acl::redis_cluster& option)
{
	const std::vector<acl::redis_node*>* nodes = option.slots();
	if (nodes == NULL)
		return false;

	std::vector<acl::redis_node*>::const_iterator cit;

	for (cit = nodes->begin(); cit != nodes->end(); ++cit)
	{
		printf("=========================================\r\n");
		printf("master: ip: %s, port: %d, slot_min: %d, slot_max: %d\r\n",
			(*cit)->get_ip(), (*cit)->get_port(),
			(int) (*cit)->get_slot_range_from(),
			(int) (*cit)->get_slot_range_to());
		print_slaves(*cit);
	}

	return true;
}

static bool preset_all(const char* addr)
{
	int max_slot = 16384;
	acl::redis_client_cluster cluster(10, 10, max_slot);
	cluster.set_all_slot(addr, 100);

	for (int i = 0; i < max_slot; i++)
	{
		acl::redis_client_pool* pool = cluster.peek_slot(i);
		if (pool == NULL)
		{
			printf("null slot: %d\r\n", i);
			return false;
		}
	}

	printf("preset all slots ok\r\n");
	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s redis_addr[127.0.0.1:6379]\r\n"
		"-C connect_timeout[default: 10]\r\n"
		"-T rw_timeout[default: 10]\r\n"
		"-a cmd[slots|preset]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, conn_timeout = 10, rw_timeout = 10;
	acl::string addr("127.0.0.1:6379"), cmd;

	while ((ch = getopt(argc, argv, "hs:n:C:T:a:")) > 0)
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
	acl::redis_cluster option(&client);

	bool ret;

	if (cmd == "slots")
		ret = test_slots(option);
	else if (cmd == "preset")
		ret = preset_all(addr.c_str());
	else
	{
		ret = false;
		printf("unknown cmd: %s\r\n", cmd.c_str());
	}

	if (ret == true)
		printf("test OK!\r\n");
	else
		printf("test failed!\r\n");

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
