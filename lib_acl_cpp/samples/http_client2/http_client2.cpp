// http_client2.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

static void get_url(const char* url, const char* host,
	const char* addr, const char* tofile)
{
	acl::http_header header;

	//header.add_param("test", NULL);
	header.set_method(acl::HTTP_METHOD_GET);
	header.set_url(url);
	header.set_content_type("text/xml; charset=utf-8");
	if (header.get_host() == NULL)
	{
		header.set_host(host);
		printf(">>>set host: %s\r\n", host);
	}
	else
		printf(">>>host: %s\r\n", header.get_host());
	header.set_keep_alive(true);
	header.add_entry("User-Agent", "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)");
	header.add_entry("Accept", "*/*");
	header.add_entry("Cache-Control", "no-cache");
	header.accept_gzip(true);

	acl::string buf;

	header.build_request(buf);
	printf("--------------------request------------------\r\n");
	printf("%s\r\n", buf.c_str());

	acl::http_client client;
	printf("begin connect %s\r\n", addr);
	if (client.open(addr, 10, 10) == false)
	{
		printf("open %s error\r\n", addr);
		return;
	}

	if (client.write_head(header) == false)
	{
		printf("write request header error\r\n");
		return;
	}
	if (client.read_head() == false)
	{
		printf("read response header error\r\n");
		return;
	}

	client.print_header("response header");

	acl::ofstream fp;
	fp.open_write(tofile);
	while (true)
	{
		int   ret = client.read_body(buf);
		if (ret < 0)
		{
			printf("connect closed now\r\n");
			break;
		}
		else if (ret == 0)
		{
			printf("over now\r\n");
			break;
		}
		fp.write(buf);
	}
}

int main(int argc, char* argv[])
{
	(void) argc; (void) argv;
	acl::acl_cpp_init(); // 必须先初始化

	//const char* url = "http://www.sina.com.cn/";
	//const char* host = "www.sina.com.cn";
	//const char* addr = "www.sina.com.cn:80";

	get_url("http://www.sina.com.cn/?name=value&nam2=value2", "www.sina.com.cn",
		"www.sina.com.cn:80", "sina.txt");
	get_url("http://www.hexun.com/", "www.hexun.com",
		"www.hexun.com:80", "hexun.txt");
	get_url("/", "www.hexun.com", "www.hexun.com:80", "hexun.txt");
	get_url("http://www.baidu.com", "www.baidu.com",
		"www.baidu.com:80", "baidu.txt");

	acl::http_header header(400);
	acl::string buf;
	header.set_content_length(1000);
	header.set_keep_alive(true);
	header.add_entry("name", "value");
	header.build_response(buf);
	printf("[%s]\r\n", buf.c_str());

	printf("enter any key to exist\r\n");
	getchar();

	return 0;
}
