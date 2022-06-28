// ssl_client.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

static acl::sslbase_conf* __ssl_conf;

static bool test(const char* addr, int k, int nloop)
{
	acl::socket_stream client;
	if (!client.open(addr, 60, 60)) {
		std::cout << "connect " << addr << " error!" << std::endl;
		exit (1);
	}

	acl::sslbase_io* ssl = __ssl_conf->create(false);
	if (client.setup_hook(ssl) == ssl) {
		std::cout << "open ssl " << addr << " error!" << std::endl;
		ssl->destroy();
		exit (1);
	}

	std::cout << "ssl handshake ok, k: " << k << std::endl;

	for (int i = 0 ; i < nloop; i++) {
		char line[1024], line2[1024];
		memset(line, 'x', sizeof(line));
		line[1023] = 0;
		line[1022] = '\n';
		if (client.write(line, strlen(line)) == -1) {
			std::cout << "write to " << addr << " error!" << std::endl;
			return false;
		}

		size_t n = sizeof(line2);
		if (!client.gets(line2, &n)) {
			std::cout << "gets from " << addr << " error!"
				<< acl_last_serror() << std::endl;
			return false;
		}
		if (memcmp(line, line2, n) != 0) {
			std::cout << "read invalid line" << std::endl;
			return false;
		}
		if (i < 1 && k < 10) {
			std::cout << ">>gets(" << n << "): " << line2 << std::endl;
		}
		if (i > 0 && i % 1000 == 0) {
			char  buf[256];
			snprintf(buf, sizeof(buf), "write count: %d", i);
			ACL_METER_TIME(buf);
		}
	}

	return true;
}

class test_thread : public acl::thread
{
public:
	test_thread(const char* addr, int k, int max)
	: addr_(addr), k_(k), max_(max) {}

	~test_thread(void) {}

protected:
	void* run(void)
	{
		(void) test(addr_, k_, max_);

		return NULL;
	}

private:
	acl::string addr_;
	int k_;
	int max_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"  -d path_to_ssl\r\n"
		"  -s server_addr[default: 127.0.0.1:9001]\r\n"
		"  -c max_connections[default: 10]\r\n"
		"  -n max_loop_per_connection[default: 10]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch, max_loop = 10, max_connections = 10;
	acl::string addr("127.0.0.1:9001"), libpath("../libpolarssl.so");

	acl::acl_cpp_init();

	while ((ch = getopt(argc, argv, "hd:s:n:c:")) > 0) {
		switch (ch) {
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

	if (libpath.find("mbedtls") != NULL) {
		const std::vector<acl::string>& libs = libpath.split2(";");
		if (libs.size() != 3) {
			printf("invalid libpath=%s\r\n", libpath.c_str());
			return 1;
		}
		acl::mbedtls_conf::set_libpath(libs[0], libs[1], libs[2]);
		if (!acl::mbedtls_conf::load()) {
			printf("load %s error\r\n", libpath.c_str());
			return 1;
		}
		__ssl_conf = new acl::mbedtls_conf(false);
	} else if (libpath.find("polarssl") != NULL) {
		acl::polarssl_conf::set_libpath(libpath);
		if (!acl::polarssl_conf::load()) {
			printf("load %s error\r\n", libpath.c_str());
			return 1;
		}
		__ssl_conf = new acl::polarssl_conf;
	}

	if (max_connections <= 0) {
		max_connections = 100;
	}

	std::vector<acl::thread*> threads;
	for (int i = 0; i < max_connections; i++) {
		acl::thread* thr = new test_thread(addr, i, max_loop);
		threads.push_back(thr);
		thr->start();
	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {
		(*it)->wait(NULL);
		delete *it;
	}
	printf("Over, enter any key to exit!\n");
	getchar();
	delete __ssl_conf;

	return (0);
}
