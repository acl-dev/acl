#include "stdafx.h"

static void usage(const char* procname) {
	printf("usage: %s -h[help]\r\n"
		"-s one_redis_addr[127.0.0.1:6379]\r\n"
		"-n loop_count[default: 10]\r\n"
		"-p password [set the password of redis cluster]\r\n"
		"-a cmd[hset|get|expire|ttl|exists|type|del]\r\n",
		procname);
}

int main(int argc, char* argv[]) {
	int  ch, count = 2;
	acl::string addr("127.0.0.1:6379"), passwd;
	acl::string cmd("del");

	while ((ch = getopt(argc, argv, "ha:s:n:p:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'a':
			cmd = optarg;
			break;
		case 's':
			addr = optarg;
			break;
		case 'n':
			count = atoi(optarg);
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
	if (!passwd.empty()) {
		pipeline.set_password(passwd);
	}
	pipeline.start_thread();

	acl::string key, name, value;

	acl::redis redis(&pipeline);
	for (int i = 0; i < count; i++) {
		key.format("hkey-%d", i);
		name.format("hname-%d", i);
		value.format("hvalue-%d", i);

		if (redis.hset(key, name, value) >= 0) {
			printf("hset %s %s %s ok\n",
				key.c_str(), name.c_str(), value.c_str());
		} else {
			printf("hset %s %s %s error\n",
				key.c_str(), name.c_str(), value.c_str());
		}
	}

	pipeline.stop_thread();
	return 0;
}
