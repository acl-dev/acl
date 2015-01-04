#include <getopt.h>
#include "acl_cpp/lib_acl.hpp"

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -s server_addr[127.0.0.1:8194] -n max_loop\r\n", procname);
}

int main(int argc, char* argv[])
{
	int  ch, max = 10;
	acl::string addr("127.0.0.1:8194");

	while ((ch = getopt(argc, argv, "hs:n:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'n':
			max = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::string buf(1024);
	for (size_t i = 0; i < 1024; i++)
		buf << 'X';

	acl::http_request req(addr, 10, 10);
	acl::string tmp(1024);

	for (int i = 0; i < max; i++)
	{
		acl::http_header& header = req.request_header();
		header.set_url("/");
		header.set_keep_alive(true);
		header.set_content_length(buf.length());

		if (req.request(buf.c_str(), buf.length()) == false)
		{
			printf("send request error\n");
			break;
		}

		if (i < 10)
			printf("send request ok\r\n");

		tmp.clear();

		int  size = 0;
		while (true)
		{
			int ret = req.read_body(tmp, false);
			if (ret < 0) {
				printf("read_body error\n");
				return 1;
			}
			else if (ret == 0)
				break;
			size += ret;
		}
		if (i < 10)
			printf(">>size: %d\n", size);
	}

	return 0;
}
