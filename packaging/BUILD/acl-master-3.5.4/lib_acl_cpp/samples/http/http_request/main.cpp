#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

static void usage(const char* procname)
{
	printf("usage: %s -h [help] \r\n"
		"\t-s server_addr[127.0.0.1:8194] \r\n"
		"\t-n max_loop \r\n"
		"\t-z[accept_gzip]\r\n"
		"\t-B[send_body data]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int  ch, max = 10;
	bool accept_gzip = false, send_body = false;
	acl::string addr("127.0.0.1:8194");

	acl::acl_cpp_init();

	while ((ch = getopt(argc, argv, "hs:n:zB")) > 0)
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
		case 'z':
			accept_gzip = true;
			break;
		case 'B':
			send_body = true;
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);

	acl::string buf(1024);
	for (size_t i = 0; i < 1024; i++)
		buf << 'X';

	acl::http_request req(addr, 10, 10);
	acl::string tmp(1024);

	for (int i = 0; i < max; i++)
	{
		acl::http_header& header = req.request_header();
		//header.set_method(acl::HTTP_METHOD_POST);
		header.set_url("/");
		header.set_keep_alive(true);
		header.accept_gzip(accept_gzip ? true : false);
		//header.set_content_length(buf.length());

		bool rc;

		if (send_body)
			rc = req.request(buf.c_str(), buf.length());
		else
			rc = req.request(NULL, 0);

		// 只所以将 build_request 放在 req.request 后面，是因为
		// req.request 内部可能会修改请求头中的字段
		acl::string hdr;
		header.build_request(hdr);
		printf("request header:\r\n%s\r\n", hdr.c_str());

		if (rc == false)
		{
			printf("send request error\n");
			break;
		}

		printf("send request ok\r\n");

		tmp.clear();

		int  size = 0, real_size = 0, n;
		while (true)
		{
			int ret = req.read_body(tmp, false, &n);
			if (ret < 0) {
				printf("read_body error\n");
				return 1;
			}
			else if (ret == 0)
			{
				printf("-------------read over---------\r\n");
				break;
			}

			size += ret;
			real_size += n;
		}

		printf("read body size: %d, real_size: %d, %s\n",
			size, real_size, tmp.c_str());

		printf("===============================================\r\n");
	}

#if defined(_WIN32) || defined(_WIN64)
	printf("Enter any key to exit ...");
	fflush(stdout);
	getchar();
#endif
	return 0;
}
