#include "stdafx.h"
#include "dns_parser.h"

static void usage(const char* procname) {
	printf("usage: %s -h[help]\r\n"
		" -s redis_addr[default: 127.0.0.1:6379]\r\n"
		" -p redis_pass[default: ""]\r\n"
		" -n count[default: 10]\r\n"
		, procname);
}

int main(int argc, char* argv[]) {
	acl::string redis_addr("127.0.0.1:6379");
	acl::string redis_pass;
	int ch, count = 10;

	while ((ch = getopt(argc, argv, "hs:p:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			redis_addr = optarg;
			break;
		case 'p':
			redis_pass = optarg;
			break;
		case 'n':
			count = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);
	acl::redis_client conn(redis_addr);
	if (!redis_pass.empty()) {
		conn.set_password(redis_pass);
	}

	dns_parser parser(conn, count);
	parser.start();
	printf("OVER!\n");

	return 0;
}
