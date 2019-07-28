// mime.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "lib_acl.h"
#ifndef WIN32
#include <getopt.h>
#endif
#include <string>
#include <errno.h>
#include <string.h>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/ifstream.hpp"
#include "acl_cpp/stream/ofstream.hpp"
#include "acl_cpp/stdlib/charset_conv.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/mime/mime.hpp"
#include "acl_cpp/mime/mime_body.hpp"
#include "acl_cpp/mime/rfc2047.hpp"
#include "acl_cpp/mime/mime_attach.hpp"

using namespace std;
using namespace acl;

static void mime_test3(acl::mime& mime, const char* path, int count)
{
	// 以下仅解析邮件头部分

	acl::string buf;

	if (acl::ifstream::load(path, &buf) == false)
	{
		printf("load %s error %s\n", path, strerror(errno));
		return;
	}

	char  info[256];
	for (int i = 0; i < count; i++)
	{
		// 开始邮件解析过程
		mime.update(buf.c_str(), buf.length());
		// 必须调用 update_end
		mime.update_end();
		mime.reset();

		if (i % 100 == 0)
		{
			snprintf(info, sizeof(info), "n: %d, i: %d, size: %ld",
				count, i, (long) buf.length());
			ACL_METER_TIME(info);
		}
	}
}

//////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{

	printf("usage: %s [options]\r\n"
		" -h [help]\r\n"
		" -n count\r\n"
		" -f mail_file\r\n", procname);
}

int main(int argc, char* argv[])
{
	char  ch;
	int   count = 1;
	acl::string path("test11.eml");

	while ((ch = (char) getopt(argc, argv, "hn:f:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return (0);
		case 'n':
			count = atoi(optarg);
			break;
		case 'f':
			path = optarg;
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);
	logger_open("test.log", "mime", "all:1");

	acl::mime mime;

	//////////////////////////////////////////////////////////////////////

	mime_test3(mime, path.c_str(), count);

	//////////////////////////////////////////////////////////////////////

	printf("enter any key to exit\r\n");
	logger_close();
	getchar();
	return 0;
}
