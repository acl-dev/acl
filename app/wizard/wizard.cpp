// wizard.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>
#if !defined(_WIN32) && !defined(_WIN64)
# include <getopt.h>
#endif
#include "master_creator.h"
#include "http_creator.h"

static void create_db()
{
}

static int create_master_service(const char* name, const char* type)
{
	master_creator(name, type);
	return 0;
}

static int create_http_service(const char* name, const char* type)
{
	http_creator(name, type);
	return 0;
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -n program_name\r\n"
		" -t service_type [ t -> master_threads, p -> master_proc, f -> master_fiber, u -> master_udp, o -> master_other ]\r\n"
		" -a application_type [ http | master, default: master ]\r\n"
		, procname);
}

int main(int argc, char* argv[])
{
	int ch;
	acl::string name, type, app("master");

	while ((ch = getopt(argc, argv, "hn:t:a:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			name = optarg;
			break;
		case 't':
			type = optarg;
			break;
		case 'a':
			app = optarg;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();

	if (!name.empty() && !type.empty()) {
		if (app == "master") {
			return create_master_service(name, type);
		} else if (app == "http") {
			return create_http_service(name, type);
		}
	}

	while (true) {
		char buf[256];

		printf("select one below:\r\n");
		printf("m: master_service; d: db; h: http; q: exit\r\n");
		printf(">"); fflush(stdout);

		int n = acl_vstream_gets_nonl(ACL_VSTREAM_IN, buf, sizeof(buf));
		if (n == ACL_VSTREAM_EOF) {
			break;
		}

		if (strcasecmp(buf, "m") == 0) {
			master_creator(NULL, NULL);
		} else if (strcasecmp(buf, "d") == 0) {
			create_db();
		} else if (strcasecmp(buf, "h") == 0) {
			http_creator(NULL, NULL);
		} else if (strcasecmp(buf, "q") == 0) {
			break;
		} else {
			printf("unknown %s\r\n", buf);
		}
	}

	printf("Bye!\r\n");
	return 0;
}
