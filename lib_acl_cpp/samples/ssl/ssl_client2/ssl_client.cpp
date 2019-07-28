// ssl_client.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include "lib_acl.h"
#include "acl_cpp/acl_cpp_init.hpp"
#include "acl_cpp/http/http_header.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/stream/polarssl_io.hpp"
#include "acl_cpp/stream/polarssl_conf.hpp"
#include "acl_cpp/http/http_client.hpp"

static acl::polarssl_conf* __ssl_conf;

static bool test(const char* addr, int k, int nloop)
{
	acl::socket_stream client;
	if (client.open(addr, 60, 60) == false)
	{
		std::cout << "connect " << addr << " error!" << std::endl;
		return false;
	}

	acl::polarssl_io* ssl = new acl::polarssl_io(*__ssl_conf, false);
	if (client.setup_hook(ssl) == ssl)
	{
		std::cout << "open ssl " << addr << " error!" << std::endl;
		ssl->destroy();
		return false;
	}

	std::cout << "ssl handshake ok, k: " << k << std::endl;

	for (int i = 0 ; i < nloop; i++)
	{
		char line[1024];
		memset(line, 'x', sizeof(line));
		line[1023] = 0;
		line[1022] = '\n';
		if (client.write(line, strlen(line)) == -1)
		{
			std::cout << "write to " << addr << " error!" << std::endl;
			return false;
		}

		size_t n = sizeof(line);
		if (client.gets(line, &n) == false)
		{
			std::cout << "gets from " << addr << " error!"
				<< acl_last_serror() << std::endl;
			return false;
		}
		if (i < 1 && k < 10)
			std::cout << ">>gets(" << n << "): " << line << std::endl;
		if (i > 0 && i % 1000 == 0)
		{
			char  buf[256];
			snprintf(buf, sizeof(buf), "write count: %d", i);
			ACL_METER_TIME(buf);
		}
	}

	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		" -d path_to_polarssl\r\n"
		"-s server_addr[default: 127.0.0.1:9001]\r\n"
		"-c max_connections[default: 10]\r\n"
		"-n max_loop_per_connection[default: 10]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch, max_loop = 10, max_connections = 10;
	acl::string addr("127.0.0.1:9001"), libpath("../libpolarssl.so");

	acl::acl_cpp_init();

	while ((ch = getopt(argc, argv, "hd:s:n:c:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'd':
			libpath = optarg;
			break;
		case 's':
			addr = optarg;
			break;
		case 'n':
			max_loop = atoi(optarg);
			break;
		case 'c':
			max_connections = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::polarssl_conf::set_libpath(libpath);
	acl::polarssl_conf::load();
	__ssl_conf = new acl::polarssl_conf;

	if (max_connections <= 0)
		max_connections = 100;

	for (int i = 0; i < max_connections; i++)
	{
		if (test(addr, i, max_loop) == false)
			break;
	}

	printf("Over, enter any key to exit!\n");
	getchar();
	delete __ssl_conf;

	return (0);
}
