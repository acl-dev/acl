#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include <stdlib.h>
#include <stdio.h>

int main(void)
{
	acl::socket_stream client;
	acl::string addr = "127.0.0.1:9001";

	acl::acl_cpp_init();
	if (client.open(addr, 0, 0) == false)
	{
		printf("open %s error\n", addr.c_str());
		getchar();
		return (0);
	}

	acl::string req = "echo1\r\n"
		"echo2\r\n"
		"echo3\r\n"
		"read_delay1\r\n"
		"read_delay2\r\n"
		"read_delay3\r\n"
		"read_delay4\r\n"
		"read_delay5\r\n"
		"quit\r\n";

	if (client.write(req.c_str(), req.length()) == -1)
	{
		printf("write error\n");
		getchar();
		return (0);
	}

	acl::string res;
	while (true)
	{
		if (client.gets(res) == false)
			break;
		printf(">>%s\r\n", res.c_str());
	}

	printf(">>stream closed, enter any key to exit ...\r\n");

	getchar();
	return (0);
}
