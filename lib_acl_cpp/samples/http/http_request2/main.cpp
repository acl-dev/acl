#include <getopt.h>
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

	for (int i = 0; i < max; i++)
	{
		acl::http_header& header = req.request_header();
		header.set_content_type("application/xml; charset=utf-8");
		header.set_url("/");
		header.set_keep_alive(true);
		header.accept_gzip(accept_gzip ? true : false);

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

		acl::string body(1024);
		if (req.get_body(body, "utf-8") == false)
		{
			printf("get_body error\r\n");
			break;
		}

		printf("read body size: %d\n", (int) body.size());

		printf("===============================================\r\n");
	}

	return 0;
}
