// zlib.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/pipe_stream.hpp"
#include "acl_cpp/stream/ofstream.hpp"
#include "acl_cpp/stream/ifstream.hpp"
#include "acl_cpp/stdlib/charset_conv.hpp"
#include "acl_cpp/stdlib/malloc.hpp"
#include "acl_cpp/stdlib/zlib_stream.hpp"

static int test_zip_stream(void)
{
	const char* dummy = "中国人民中国人民中国人民中国人民中国人民\r\n";
	acl::zlib_stream zstream;
	acl::string out, out2;

	const char* ptr = dummy;
	int  n = (int) strlen(ptr);

	bool ret = zstream.zip_begin(acl::zlib_level9);
	if (ret == false)
	{
		printf("zip_begin error\r\n");
		zstream.zip_reset();
		return 1;
	}
	while (n > 0)
	{
		ret = zstream.zip_update(ptr, n, &out, acl::zlib_flush_off);
		if (ret == false)
		{
			printf("zip_update error\r\n");
			return 1;
		}
		ptr++;
		n--;
		n = 0;
	}
	ret = zstream.zip_finish(&out);
	if (ret == false)
	{
		printf("zip_finish error\r\n");
		return 1;
	}

	printf(">>after zip, src's len: %d, dst's len: %d\r\n",
		(int) strlen(dummy), (int) out.length());

	ptr = out.c_str();
	n = (int) out.length();

	ret = zstream.unzip_begin();
	if (ret == false)
	{
		printf("unzip_begin error\r\n");
		return 1;
	}

	while (n > 0)
	{
		ret = zstream.unzip_update(ptr, 1, &out2, acl::zlib_flush_full);
		if (ret == false)
		{
			printf("unzip_update error\r\n");
			return 1;
		}
		ptr++;
		n--;
	}
	ret = zstream.unzip_finish(&out2);
	if (ret == false)
	{
		printf("unzip_finish error\r\n");
		return 1;
	}

	printf("len: %d, out2: %s", (int) out2.length(), out2.c_str());
	return 0;
}

static int test_compress(void)
{
	printf("-----------------------------------------------\r\n");

	const char* dummy = "中国人民中国人民中国人民中国人民中国人民\r\n";
	acl::zlib_stream zstream;
	acl::string out, out2;

	if (zstream.zlib_compress(dummy, (int) strlen(dummy),
		&out, acl::zlib_best_compress) == false)
	{
		printf("zlib_compress error\r\n");
		return 1;
	}

	printf(">>after compress, len: %d\r\n", (int) out.length());

	if (zstream.zlib_uncompress(out.c_str(), (int) out.length(), &out2) == false)
	{
		printf("zlib_uncompress error\r\n");
		return 1;
	}

	printf(">>len: %d, result: %s\r\n", (int) out2.length(), out2.c_str());
	return 0;
}

static void test_zlib_pipe(void)
{
	acl::ifstream in;

	if (in.open_read("./in.txt") == false)
	{
		logger_error("open in.txt error");
		return;
	}

	acl::pipe_manager manager;
	acl::charset_conv gbToUtf;
	acl::zlib_stream zip;
	acl::zlib_stream unzip;
	acl::charset_conv utfToGb;
	acl::ofstream out;
	acl::pipe_string ps;

	zip.pipe_zip_begin();
	unzip.pipe_unzip_begin();
	if (1)
	{
		gbToUtf.update_begin("gb2312", "utf-8");
		utfToGb.update_begin("utf-8", "gb2312");
	}
	else
	{
		gbToUtf.update_begin("gb2312", "big5");
		utfToGb.update_begin("big5", "gb2312");
	}

	if (out.open_write("./out.txt") == false)
	{
		logger_error("open out.txt file error");
		return;
	}

	if (0)
	{
		// 倒序方式添加，即执行的顺序与添加的顺序相反

		manager.push_front(&ps);
		manager.push_front(&out);
		manager.push_front(&utfToGb);
		manager.push_front(&unzip);
		manager.push_front(&zip);
		manager.push_front(&gbToUtf);
	}
	else
	{
		// 正序方式添加，即执行的顺序与添加的顺序相同

		manager.push_back(&gbToUtf);
		manager.push_back(&zip);
		manager.push_back(&unzip);
		manager.push_back(&utfToGb);
		manager.push_back(&ps);
		manager.push_back(&out);
	}

	while (true)
	{
		char buf[1];
		int  ret;
		if ((ret = in.read(buf, sizeof(buf), false)) == -1)
			break;

		if (manager.update(buf, ret) == false)
		{
			logger_error("manager update error");
			return;
		}
	}

	if (manager.update_end() == false)
		logger_error("manager update_end error");

	printf("result >>> %s\r\n", ps.c_str());

	//while (true)
	//{
	//	char buf[1];
	//	acl::string out_buf;
	//	int  ret;

	//	if ((ret = in.read(buf, sizeof(buf), false)) == -1)
	//		break;
	//	if (utfToGb.update(buf, ret, &out_buf) == false)
	//	{
	//		logger_error("charset update error");
	//		break;
	//	}

	//	out.write(out_buf.c_str(), out_buf.length());
	//	out_buf.clear();
	//}
}

static void check(void)
{
	printf("enter any key to exit now\r\n");
	getchar();
}

static void test_unzip_file(void)
{
	const char* file = "test.gz";
	acl::string buf;
	if (acl::ifstream::load(file, &buf) == false)
		return;
	const char* ptr = buf.c_str() + 10;
	acl::zlib_stream zstream;
	if (zstream.unzip_begin(false) == false)
		return;
	acl::string out;
	if (zstream.unzip_update(ptr, (int) buf.length() - 10, &out) == false)
		printf("unzip_update error\r\n");
	else if (zstream.unzip_finish(&out) == false)
	{
		printf(">>%s\r\n", out.c_str());
		acl::ofstream fp;
		fp.open_write("result.txt");
		fp.write(out);
		printf("unzip_finish error\r\n");
	}
	else
		printf("unzip_finish ok\r\n");
}

static void test_pipe()
{
	acl::pipe_manager manager;
	acl::pipe_string ps1, ps2, ps3;

	manager.push_back(&ps1);
	manager.push_back(&ps2);
	manager.push_back(&ps3);

	for (int i = 0; i < 4096; i++)
		manager.update("x", 1);
	manager.update("", 1);
	manager.update_end();
	printf("result >>%s\r\n", ps3.c_str());
}

int main(int argc, char* argv[])
{
	acl::log::stdout_open(true);

	test_pipe();

	(void) argc;
	(void) argv;

	atexit(check);

	//  win32 下，在 DLL 中不得使用内存池功能
#ifdef  USE_SLICE
# ifndef ACL_DLL
	acl::acl_slice_init();
# endif
#endif

	test_unzip_file();

	int ret1 = 0, ret2 = 0;

	ret1 = test_zip_stream();
	ret2 = test_compress();
	test_zlib_pipe();

	printf("enter any key to call functions registered by atexit\r\n");
	getchar();
	return (ret1 == 0 && ret2 == 0 ? 0 : 1);
}
