#include <assert.h>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include <stdio.h>

static int test1(void)
{
	const char* s = "中国人民共和国";
	acl::charset_conv conv;
	acl::string out, out2, out3;
	acl::fstream out_fp;
	acl::mime_base64 encoder;

	/////////////////////////////////////////////////////////////////////


	if (conv.convert("gb2312", "utf-8", s, strlen(s), &out) == false)
	{
		printf("convert from gb2312 to utf-8 error\n");
		getchar();
		return (1);
	}
	printf("ok, gb2312 to utf-8(%d): %s\n\n", (int) out.length(), out.c_str());

	if (out_fp.open_trunc("gbutf8_1.txt"))
	{
		out_fp << out.c_str();
		out_fp.close();
	}

	///

	encoder.encode_update(out.c_str(), out.length(), &out3);
	encoder.encode_finish(&out3);
	printf("ok, utf-8's base64: %s\n", out3.c_str());

	if (out_fp.open_trunc("gbutf8.txt"))
	{
		out_fp << (char) 0xef << (char) 0xbb << (char) 0xbf
			<< out.c_str() << "\r\n";
		out_fp << out.c_str();
		out_fp.close();
	}

	if (conv.convert("utf-8", "gb2312", out, out.length(), &out2) == false)
	{
		printf("convert from utf-8 to gb2312 error\n");
		getchar();
		return (1);
	}
	printf("ok, utf-8 to gb2312: %s\n", out2.c_str());

	/////////////////////////////////////////////////////////////////////

	out.clear();
	if (conv.convert("gb2312", "big5", s, strlen(s), &out) == false)
	{
		printf("convert from gb2312 to big5 error\n");
		getchar();
		return (1);
	}
	printf("ok, gb2312 to big5: %s\n", out.c_str());

	out3.clear();
	encoder.encode_update(out.c_str(), out.length(), &out3);
	encoder.encode_finish(&out3);
	printf("ok, big5's base64: %s\n", out3.c_str());

	if (out_fp.open_trunc("big5.txt"))
	{
		out_fp << out.c_str();
		out_fp.close();
	}

	out2.clear();
	if (conv.convert("big5", "gb2312", out, out.length(), &out2) == false)
	{
		printf("convert from big5 to gb2312 error\n");
		getchar();
		return (1);
	}
	printf("ok, big5 to gb2312: %s\n", out2.c_str());

	/////////////////////////////////////////////////////////////////////

	out.clear();
	if (conv.convert("gb2312", "hz", s, strlen(s), &out) == false)
	{
		printf("convert from gb2312 to hz error\n");
		getchar();
		return (1);
	}
	printf("ok, gb2312 to hz: %s, %d, %d\n", out.c_str(), (int) strlen(s),
		(int) out.length());

	out2.clear();
	if (conv.convert("hz", "gb2312", out, out.length(), &out2) == false)
	{
		printf("convert from hz to gb2312 error\n");
		getchar();
		return (1);
	}
	printf("ok, hz to gb2312: %s, len: %d\n", out2.c_str(), (int) out2.length());

	///////////////////////    流式解析过程测试   ///////////////////////

	/////////////////////////////////////////////////////////////////////

	out.clear();
	const char* ptr = s;

	if (conv.update_begin("GB2312", "UTF-8") == false)
		return (1);
	conv.update(ptr, strlen(ptr), &out);
	conv.update_finish(&out);
	printf(">> from gdb2312 to utf-8: %s\n\n", out.c_str());

	/*
	out2.clear();
	if (conv.update_begin("UNICODE", "UTF-8") == false)
		return (1);
	conv.update(out.c_str(), out.length(), &out2);
	conv.update_finish(&out2);

	printf(">> from gb2312 to utf-8: %s\n", out2.c_str());
	return (0);
	*/

	out2.clear();
	out << " hello world!";
	ptr = out.c_str();
	if (conv.update_begin("utf-8", "gb2312") == false)
		return (1);
	while (*ptr)
	{
		conv.update(ptr, strlen(ptr), &out2);
		break;
		ptr++;
	}
	conv.update_finish(&out2);
	printf(">> from utf-8 to gb2312: %s\n", out2.c_str());

	/////////////////////////////////////////////////////////////////////

	out.clear();
	ptr = s;

	if (conv.update_begin("GB2312", "BIG5") == false)
		return (1);
	conv.update(ptr, strlen(ptr), &out);
	conv.update_finish(&out);
	printf(">> from gdb2312 to big5: %s\n", out.c_str());

	out2.clear();
	out << " hello world!";
	ptr = out.c_str();
	if (conv.update_begin("BIG5", "GB2312") == false)
		return (1);
	while (*ptr)
	{
		conv.update(ptr, strlen(ptr), &out2);
		break;
		ptr++;
	}
	conv.update_finish(&out2);
	printf(">> from BIG5 to gb2312: %s\n", out2.c_str());

	/////////////////////////////////////////////////////////////////////

	getchar();
	return (0);
}

static int test2(const char* filepath)
{
	acl::string buf1;

	if (acl::ifstream::load(filepath, &buf1) == false)
	{
		printf("load %s error\n", filepath);
		return (-1);
	}

	acl::charset_conv conv;

	acl::string buf2;
	if (conv.convert("gb18030", "utf-8", buf1.c_str(),
		buf1.length(), &buf2) == false)
	{
		printf("convert from gb18030 to utf-8 error\n");
		return (-1);
	}

	acl::string buf3;
	if (conv.convert("utf-8", "gb18030", buf2.c_str(),
		buf2.length(), &buf3) == false)
	{
		printf("convert from utf-8 to gb18030 error\n");
		return (-1);
	}

	if (buf1 != buf3)
	{
		printf("convert failed\n");
		assert(0);
		return (-1);
	}
	else
	{
		printf("convert ok(tid: %u)\n", (unsigned int) acl::thread::thread_self());
		return (0);
	}
}

static void test3_thread_callback(void* ctx)
{
	const char* filepath = (const char*) ctx;

	test2(filepath);
}

static int test3(char* filepath, int nthreads, int count)
{
	acl_pthread_pool_t* pool = acl_thread_pool_create(nthreads, 60);

	for (int i = 0; i < count; i++)
		acl_pthread_pool_add(pool, test3_thread_callback, filepath);

	printf("Over now, current threads: %d\n", acl_pthread_pool_size(pool));
	acl_pthread_pool_destroy(pool);
	return (0);
}

static void test_unicode(void)
{
	acl::charset_conv conv;
	acl::string in, out;
	unsigned char  i8;
	unsigned short i16;
	int   i32, k = 0;
	bool  ret;

	goto TAG;

	i16 = 32;
	in.copy((const void*) &i16, 2);
	ret = conv.convert("UNICODE", "gb18030", in.c_str(), in.length(), &out);
	if (ret == true)
	{
		if (out.length() == 1)
		{
			memcpy(&i8, out.c_str(), out.length());
			k = i8;
		}
		else if (out.length() == 2)
		{
			memcpy(&i16, out.c_str(), out.length());
			k = i16;
		}
		else if (out.length() > 2)
		{
			k = out.length() > sizeof(i32) ? sizeof(i32) : out.length();
			memcpy(&i32, out.c_str(), k);
			k = i32;
		}

		printf(">>result: %d, len: %d, %c%c\n", k, (int) out.length(), (int) k >> 8, k & 0xff);
	}
	else
		printf("convert error\n");

	printf("-------------------\n");

TAG:

	i16 = 0xc2f3;
	i16 = 33296;
#if 0
	i16 = 17320;
	i16 = 53558;
	i16 = 43681;
	ret = conv.convert("gb18030", "UCS-2", (char*) &i16, 2, &out);
#else	
	i16 = 8211;
	i16 = 8212;
	i16 = 8801;
	ret = conv.convert("UCS-2", "gb18030", (char*) &i16, 2, &out);
#endif

	printf(">>>>8801: %c\n", (unsigned char) 34);
	if (ret == true)
	{
		if (out.length() == 1)
		{
			memcpy(&i8, out.c_str(), out.length());
			k = i8;
		}
		else if (out.length() == 2)
		{
			memcpy(&i16, out.c_str(), out.length());
			k = i16;
		}
		else if (out.length() > 2)
		{
			k = out.length() > sizeof(i32) ? sizeof(i32) : out.length();
			memcpy(&i32, out.c_str(), k);
			printf(">>>k: %u, %s\n", k, out.c_str());
			k = i32 >> 24 | ((i32 >> 16) & 0xff) << 8 | ((i32 >>8) & 0xff) << 16 | (i32&0xff) << 24;
			printf(">>>k2: %u, i32: %u\n", k, i32);
		}

		printf(">>result: %u, len: %d, %c%c\n", (unsigned short) k, (int) out.length(), (int) k >> 8, k & 0xff);
	}
	else
		printf("convert error\n");
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -t test1|test2|test3 -f filepath -c nthreads -n count\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch, nthreads = 1, count = 10;
	char  buf[256], filepath[256];

	if (0)
	{
		test_unicode();
		return 0;
	}

	filepath[0] = 0;
	while ((ch = getopt(argc, argv, "ht:f:n:c:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return (0);
		case 't':
			snprintf(buf, sizeof(buf), "%s", optarg);
			break;
		case 'f':
			snprintf(filepath, sizeof(filepath), "%s", optarg);
			break;
		case 'n':
			count = atoi(optarg);
			break;
		case 'c':
			nthreads = atoi(optarg);
			break;
		default:
			 break;
		}
	}

	if (strcasecmp(buf, "test1") == 0)
		return (test1());
	else if (strcasecmp(buf, "test2") == 0)
	{
		if (filepath[0] == 0)
		{
			usage(argv[0]);
			return (1);
		}
		return (test2(filepath));
	}
	else if (strcasecmp(buf, "test3") == 0)
	{
		if (filepath[0] == 0)
		{
			usage(argv[0]);
			return (1);
		}
		if (nthreads <= 0)
			nthreads = 1;
		if (count <= 10)
			count = 10;
		return (test3(filepath, nthreads, count));
	}
	else
		usage(argv[0]);
	return (0);
}
