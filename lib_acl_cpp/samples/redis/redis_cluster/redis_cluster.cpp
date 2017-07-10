#include "stdafx.h"

static void print_slaves_slots(const acl::redis_slot* slot)
{
	const std::vector<acl::redis_slot*>& slaves = slot->get_slaves();
	std::vector<acl::redis_slot*>::const_iterator cit;
	for (cit = slaves.begin(); cit != slaves.end(); ++cit) {
		printf(
			"slave: ip: %s, port: %d, slot_min: %d, slot_max: %d\r\n",
			(*cit)->get_ip(), (*cit)->get_port(),
			(int) (*cit)->get_slot_min(), (int) (*cit)->get_slot_max());
	}
}

static bool test_slots(acl::redis_cluster& redis)
{
	const std::vector<acl::redis_slot*>* slots = redis.cluster_slots();
	if (slots == NULL)
		return false;

	std::vector<acl::redis_slot*>::const_iterator cit;

	for (cit = slots->begin(); cit != slots->end(); ++cit) {
		printf("=========================================\r\n");
		printf(
			"master: ip: %s, port: %d, slot_min: %d, slot_max: %d\r\n",
			(*cit)->get_ip(), (*cit)->get_port(),
			(int) (*cit)->get_slot_min(), (int) (*cit)->get_slot_max());
		print_slaves_slots(*cit);
	}

	return true;
}

static void print_master_slots(const acl::redis_node* master)
{
	const std::vector<std::pair<size_t, size_t> >& slots = master->get_slots();
	std::vector<std::pair<size_t, size_t> >::const_iterator cit;
	for (cit = slots.begin(); cit != slots.end(); ++cit)
		printf("slots range: %d-%d\r\n", (int) (*cit).first,
			(int) (*cit).second);
}

static void print_slave_nodes(const std::vector<acl::redis_node*>& slaves)
{
	std::vector<acl::redis_node*>::const_iterator cit;
	for (cit = slaves.begin(); cit != slaves.end(); ++cit) {
		printf("slave, id: %s, addr: %s, master_id: %s\r\n", (*cit)->get_id(),
			(*cit)->get_addr(), (*cit)->get_master_id());
	}
}

static bool test_nodes(acl::redis_cluster& redis)
{
	const std::map<acl::string, acl::redis_node*>* masters = redis.cluster_nodes();
	if (masters == NULL)
		return false;

	std::map<acl::string, acl::redis_node*>::const_iterator cit;
	for (cit = masters->begin(); cit != masters->end(); ++cit) {
		printf("==========================================\r\n");
		printf("master, id: %s, addr: %s\r\n", cit->first.c_str(),
			cit->second->get_addr());
		print_master_slots(cit->second);
		const std::vector<acl::redis_node*>* slaves = cit->second->get_slaves();
		print_slave_nodes(*slaves);
	}

	return true;
}

static bool test_slaves(acl::redis_cluster& redis, const char* node)
{
	const std::vector<acl::redis_node*>* slaves = redis.cluster_slaves(node);
	if (slaves == NULL)
		return false;

	print_slave_nodes(*slaves);
	return true;
}

static bool test_info(acl::redis_cluster& redis)
{
	std::map<acl::string, acl::string> result;
	if (redis.cluster_info(result) == false)
		return false;

	std::map<acl::string, acl::string>::const_iterator cit;
	for (cit = result.begin(); cit != result.end(); ++cit)
		printf("%s=%s\r\n", cit->first.c_str(), cit->second.c_str());

	return true;
}

static bool preset_all(const char* addr)
{
	int max_slot = 16384;
	acl::redis_client_cluster cluster(max_slot);
	cluster.set_all_slot(addr, 100);

	for (int i = 0; i < max_slot; i++) {
		acl::redis_client_pool* pool = cluster.peek_slot(i);
		if (pool == NULL) {
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
	       "-i node_id[for action: slaves]\r\n"
	       "-a cmd[slots|nodes|slaves|info|preset]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int ch, conn_timeout = 10, rw_timeout = 10;
	acl::string addr("127.0.0.1:6379"), cmd, node;

	while ((ch = getopt(argc, argv, "hs:n:C:T:a:i:")) > 0) {
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
		case 'i':
			node = optarg;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::redis_client client(addr.c_str(), conn_timeout, rw_timeout);
	acl::redis_cluster redis(&client);

	bool ret;

	if (cmd == "slots")
		ret = test_slots(redis);
	else if (cmd == "preset")
		ret = preset_all(addr.c_str());
	else if (cmd == "nodes")
		ret = test_nodes(redis);
	else if (cmd == "slaves") {
		if (node.empty()) {
			printf("usage: %s -a slaves -i node\r\n", argv[0]);
			return 1;
		}
		ret = test_slaves(redis, node.c_str());
	} else if (cmd == "info")
		ret = test_info(redis);
	else {
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
