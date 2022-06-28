#include "stdafx.h"

static void show_slave(const acl::redis_role4slave& slave)
{
	printf("slave:\r\n");
	printf("\tip: %s\r\n", slave.get_ip());
	printf("\tport: %d\r\n", slave.get_port());
	printf("\tstatus: %s\r\n", slave.get_status());
	printf("\toffset: %lld\r\n", slave.get_offset());
}

static void show_master(const acl::redis_role4master& master)
{
	printf("master node:\r\n");
	printf("\toffset=%lld\r\n", master.get_offset());

	const std::vector<acl::redis_role4slave>& slaves = master.get_slaves();
	for (std::vector<acl::redis_role4slave>::const_iterator
			cit = slaves.begin(); cit != slaves.end(); ++cit) {
		show_slave(*cit);
	}
}

static bool test(acl::redis_role& cmd)
{
	if (!cmd.role()) {
		printf("role error=%s\r\n", cmd.result_error());
		return false;
	}
	printf("call role ok\r\n");

	const char* role_name = cmd.get_role_name();
	if (role_name == NULL || *role_name == 0) {
		printf("role_name null\r\n");
		return false;
	}

	if (strcasecmp(role_name, "master") == 0) {
		const acl::redis_role4master& master = cmd.get_role4master();
		show_master(master);
	} else if (strcasecmp(role_name, "slave") == 0) {
		const acl::redis_role4slave& slave = cmd.get_role4slave();
		printf("slave:\r\n");
		printf("\tmaster ip: %s\r\n", slave.get_ip());
		printf("\tmaster port: %d\r\n", slave.get_port());
		printf("\tstatus: %s\r\n", slave.get_status());
		printf("\toffset: %lld\r\n", slave.get_offset());
	}

	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s redis_addr[127.0.0.1:6379]\r\n"
		"-p password[default: \"\"]\r\n"
		"-C connect_timeout[default: 10]\r\n"
		"-T rw_timeout[default: 10]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, conn_timeout = 10, rw_timeout = 10;
	acl::string addr("127.0.0.1:6379"), passwd;

	while ((ch = getopt(argc, argv, "hs:C:T:p:")) > 0) {
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
		case 'p':
			passwd = optarg;
			break;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl::redis_client client(addr.c_str(), conn_timeout, rw_timeout);
	client.set_password(passwd);

	acl::redis_role cmd(&client);
	test(cmd);

	return 0;
}
