// socket_stream.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "lib_acl.h"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/socket_stream.hpp"

//using acl;

int main(int argc, char* argv[])
{
	(void) argc;
	(void) argv;

	acl::acl_cpp_init();
	const char* local_addr = "127.0.0.1:8088";
	acl::server_socket server(acl::OPEN_FLAG_EXCLUSIVE, 128);
	if (server.open(local_addr) == false) {
		printf("listen %s error %s\r\n", local_addr, acl::last_serror());
		return 1;
	}
	printf("listen %s ok\r\n", local_addr);

	acl::socket_stream client;
	acl::string addr = "mail.51iker.com:80";
	acl::string request = "GET / HTTP/1.1\r\n"
		"HOST: mail.51iker.com\r\n"
		"Connection: close\r\n\r\n";
	acl::string respond;

	acl::acl_cpp_init();

	if (client.open(addr, 0, 0) == false) {
		printf("connect %s error(%s)\n", addr.c_str(), acl_last_serror());
		printf("input any key to exit\n");
		getchar();
		return (0);
	}

	if (client.write(request) == -1) {
		printf("write %s to %s error(%s)\n", request.c_str(), addr.c_str(),
			acl_last_serror());
		printf("input any key to exit\n");
		getchar();
	}
	printf("%s", request.c_str());

	while (1) {
		if (client.read(respond, false) == false)
			break;
		printf("%s", respond.c_str());
	}

	printf("ok, enter any key to exit\n");
	getchar();
	return 0;
}
