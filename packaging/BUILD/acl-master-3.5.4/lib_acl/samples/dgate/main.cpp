// DnsGateway.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "configure.h"
#include "service_main.h"

static SERVICE *__service;

static const char *__conf_file = "dgate.cf";

static void init(void)
{
	acl_socket_init();
	conf_load(__conf_file);

	printf("local port: %d\r\n", var_cfg_server_port);

	//acl_msg_open("dgate.log", "dgate");
	acl_msg_stdout_enable(1);

	__service = service_create("0.0.0.0", (short) var_cfg_server_port,
		var_cfg_dns_neighbor_ip, var_cfg_dns_neighbor_port);

	printf("neighbor dns_ip: %s, dns_port: %d\r\n",
		var_cfg_dns_neighbor_ip, var_cfg_dns_neighbor_port);
}

static void run(void)
{
	service_start(__service);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -f conf_file\r\n", procname);
}

int main(int argc, char* argv[])
{
	char  ch;

	while ((ch = getopt(argc, argv, "hf:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			__conf_file = acl_mystrdup(optarg);
			break;
		default:
			break;
		}
	}

	init();
	run();
	return 0;
}
