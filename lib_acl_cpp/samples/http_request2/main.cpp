// http_servlet.cpp : 定义控制台应用程序的入口点。
//
#include <assert.h>
#include <getopt.h>
#include "acl_cpp/lib_acl.hpp"

using namespace acl;

//////////////////////////////////////////////////////////////////////////

class http_request_test
{
public:
	http_request_test(const char* server_addr,
		const char* stype, const char* charset)
	{
		server_addr_= server_addr;
		stype_ = stype;
		charset_ = charset;
		to_charset_ = "gb2312";
	}

	~http_request_test() {}

	bool run(void)
	{
		http_request req(server_addr_);

		// 添加 HTTP 请求头字段

		string ctype("text/");
		ctype << stype_ << "; charset=" << charset_;

		http_header& hdr = req.request_header();  // 请求头对象的引用
		hdr.set_url("/");
		hdr.set_content_type(ctype);

		// 发送 HTTP 请求数据
		if (req.request(NULL, 0) == false)
		{
			logger_error("send http request to %s error",
				server_addr_.c_str());
			return false;
		}

		acl::string line;

		while (true)
		{
			if (req.body_gets(line) == false)
				break;
			printf(">>>[%s], len: %d\r\n", line.c_str(),
				(int) line.size());
			line.clear();
		}

		if (req.body_gets(line))
			printf(">>last data: %s\r\n", line.c_str());

		return true;
	}

private:
	string server_addr_;	// web 服务器地址
	string stype_;		// 请求数据的子数据类型
	string charset_;	// 本地请求数据文件的字符集
	string to_charset_;	// 将服务器响应数据转为本地字符集
};

//////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -s server_addr\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch;
	string server_addr("127.0.0.1:8888");
	string stype("html"), charset("gb2312");

	while ((ch = getopt(argc, argv, "hs:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			server_addr = optarg;
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	// 开始运行
	log::stdout_open(true);
	http_request_test test(server_addr, stype, charset);
	test.run();

	return 0;
}

