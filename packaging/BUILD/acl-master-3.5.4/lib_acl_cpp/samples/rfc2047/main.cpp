#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef WIN32
//#include "vld.h"  // win32 下进行内存泄露检测
#include "lib_acl.h"
#else
#include <getopt.h>
#endif
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/fstream.hpp"
#include "acl_cpp/stream/ifstream.hpp"
#include "acl_cpp/stream/ofstream.hpp"
#include "acl_cpp/mime/rfc2047.hpp"

static int isvalidChinese(char c1, char c2)
{
	if ((c1 & 0x80) == 0 || (c2 & 0x80) == 0)
		return (0);
	else
		return (1);
	if (!((c2)&0xc0) )
		return (0);
}

// 获得有效的汉字字符
static int get_valid_string(char *src, unsigned dlen, char *buffer, unsigned bsize)
{
	int n = 0;
	unsigned char *src_ptr = NULL;
	unsigned char *obj_ptr = NULL;
	unsigned char *ch = NULL;
	int firstChineseChar = 0;
	char lastChar = 0;

	if (src == NULL || dlen == 0 || buffer == NULL || bsize == 0)
		return -1;

	obj_ptr = (unsigned char *)buffer;

	src_ptr = (unsigned char *)src;

	/* 为最后一个 '\0' 留出一个字节空间 */
	n =  bsize - 1;

	for (ch = src_ptr; *ch; ch++) {
		if (ch - src_ptr >= n)
			break;

		/* 先判断前一个字节最高位是否为 1 */
		if (firstChineseChar) {
			if (!isvalidChinese(lastChar,*ch))
				break;

			//if (ch - src_ptr >= n + 1)
			//	break;

			firstChineseChar = 0; 
			/* 先拷贝前一个字节 */
			*obj_ptr++ = lastChar;
			*obj_ptr++ = *ch;
		}
		else if ((*ch) & 0x80) {
			/* 说明当前字节的最高位为 1, 且有可能是汉字的第一个字节 */
			firstChineseChar = 1;
			lastChar = *ch;
		}
		else
			*obj_ptr++ = *ch;
	}
	*obj_ptr = 0;
	return 0;
}

static void rfc2047_test(acl::rfc2047& rfc2047, const char* s)
{
	acl::string out;

	printf("\r\n\r\n");
	printf("------------------ src --------------------------------\r\n");
	printf("%s\r\n", s);
	printf("------------------ charset to gb2312 begin ------------\r\n");
	rfc2047.reset(true);
	rfc2047.decode_update(s, (int) strlen(s));
	if (rfc2047.decode_finish("gb18030", &out))
	{
		printf(">>>before valid |%s|, len(%d)\r\n", out.c_str(), (int) out.size());

		char buf[50];

		get_valid_string(out.c_str(), (unsigned) out.length(),
			buf, (unsigned) sizeof(buf) - 3);
		printf(">>>after valid |%s|, len(%d)\n", buf, (int) strlen(buf));

		rfc2047.debug_rfc2047();
	}
	else
		printf(">>error: %s\n", out.c_str());
	printf("------------------ charset to gb2312 end ---------------\n");
}

static void rfc2047_test2(acl::rfc2047& rfc2047, const char* s)
{
	acl::string out;

	printf("\n");
	printf("------------------ charset to utf-8 begin --------------\n");
	rfc2047.reset(true);
	rfc2047.decode_update(s, (int) strlen(s));
	if (rfc2047.decode_finish("utf-8", &out))
	{
		printf(">>> {%s}\n", out.c_str());
		rfc2047.debug_rfc2047();

		acl::fstream fp;
		if (fp.open_trunc("test.html"))
		{
			fp << "<html xmlns=\"http://www.w3.org/1999/xhtml\">\r\n"
				<< "<head>\r\n"
				<< "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\r\n"
				<< "</head>\r\n"
				<< "<body>\r\n";
			fp.write(out.c_str(), out.length());
			fp << "</body>\r\n</html>\r\n";
		}
	}
	else
		printf(">>error: %s\n", out.c_str());
	printf("------------------ charset to utf-8 end ----------------\n");
}

static void rfc2047_test_encode(const char* s)
{
	acl::string out;
	acl::rfc2047 rfc2047(false, false);

	rfc2047.encode_update(s, (int) strlen(s), &out);
	rfc2047.encode_update(s, (int) strlen(s), &out, "gb2312", 'Q');
	rfc2047.encode_finish(&out);
	printf("rfc2047 encoding result: %s\n", out.c_str());
}

static void rfc2047_test_file(acl::rfc2047& rfc2047, const char* filepath)
{
	acl::string buf;

	if (acl::ifstream::load(filepath, &buf) == false)
	{
		printf("load file %s error(%s)\r\n", filepath, strerror(errno));
		return;
	}

	printf("-----------------------input data----------------------\r\n");
	printf(">>src: \r\n|%s|\r\n", buf.c_str());
	printf("-----------------------input data end------------------\r\n");

	rfc2047.decode_update(buf.c_str(), (int) buf.length());

	buf.clear();
	rfc2047.decode_finish("gb18030", &buf);
	printf(">>result:\r\n|%s|\r\n", buf.c_str());

	acl::ofstream out;
	if (out.open_write("out.txt") == false)
	{
		printf("open out.txt error\r\n");
		return;
	}

	out.write(buf);

	const std::list<acl::rfc2047_entry*>& entries = rfc2047.get_list();
	std::list<acl::rfc2047_entry*>::const_iterator cit = entries.begin();
	for (; cit != entries.end(); cit++)
	{
		if ((*cit)->pCharset && !(*cit)->pCharset->empty())
		{
			if ((*cit)->pData)
				printf("data: %s, ", (*cit)->pData->c_str());
			if ((*cit)->pCharset)
				printf("charset: %s, ", (*cit)->pCharset->c_str());
			printf("code: %c\r\n", (*cit)->coding);
		}
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -d src -e src -f file\n", procname);
}

int main(int argc, char* argv[])
{
	acl::rfc2047 rfc2047;

	int   ch;

	while ((ch = getopt(argc, argv, "hd:e:f:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return (0);
		case 'd':
			rfc2047_test(rfc2047, optarg);
			return (0);
		case 'e':
			rfc2047_test_encode(optarg);
			return (0);
		case 'f':
			rfc2047_test_file(rfc2047, optarg);
			return (0);
		default:
			break;
		}
	}

	/////////////////////////////////////////////////////////////////////

	const char* s = "[=?UTF-8?Q?=E7=BC=BA=E9=99=B7=20602?=]=?UTF-8?Q?=20=E6=89=80=E5=B1=9E=E9=83=A8=E9=97=A8=E5=BA=94=E4=B8=BA=E4=B8=AD=E6=96=87?=";
	acl::string out;
	const char* ptr = s, *end = s + strlen(s);

	while (ptr < end)
	{
		rfc2047.decode_update(ptr, 1);
		ptr++;
	}

	if (rfc2047.decode_finish("gb2312", &out))
	{
		printf(">>ok\n");
		printf("%s\n", out.c_str());
	}
	else
		printf(">>error: %s\n", out.c_str());

	/////////////////////////////////////////////////////////////////////

	s = "=?GB2312?B?yO28/r+qt6IyLjC087vh\r\n"
		"  IM3FubrT\r\n"
		"   xbvd1tA=?=";
	rfc2047_test(rfc2047, s);

	/////////////////////////////////////////////////////////////////////

	s = "Re: [hiphop-php-dev] Why does hiphop delete my output directory?";
	rfc2047_test(rfc2047, s);

	/////////////////////////////////////////////////////////////////////

	s = " =?UTF-8?\r\n"
		" Q?<=E5=B9=BF=E5=91=8A>_iPho?=\r\n"
		" =?UTF\r\n"
		" -8?Q?ne_4_=E7=8E=B0=E5=B7\r\n"
		" =B2=E5=8F=91=E5=94=AE?=";
	rfc2047_test(rfc2047, s);

	/////////////////////////////////////////////////////////////////////

	s = " =\r\n"
		" ?\r\n"
		" U\r\n"
		" T\r\n"
		" F\r\n"
		" -8?\r\n"
		" Q?\r\n"
		" <=E5=B9=BF=E5=91=8A>\r\n"
		" _iPho?\r\n"
		" =\r\n"
		" =?UTF\r\n"
		" -8?\r\n"
		" Q?ne_4_=E7\r\n"
		" =8E=B0=E5=B7\r\n"
		" =B2=E5=8F=91=E5=94=AE?\r\n"
		" =";
	rfc2047_test(rfc2047, s);

	/////////////////////////////////////////////////////////////////////
	
	s = "\"=?gb2\r\n312?B?yMvBpt\r\nfK1\r\nLSyvw=\r\n=\r\n?=\" <hr@51iker.com>";
	rfc2047_test(rfc2047, s);

	/////////////////////////////////////////////////////////////////////
	
#if 1
	const char* s1 = "=?gb2312?B?ILL9wfrN+KOsyei8xqOsxr3D5snovMajrLmk0rXJ6LzGo6zN+NKzyei8xqOssg==?="
		"=?GB2312?B?y8bXyei8xqOssvrGt8novMajrNPK1rfTys/kt/7O8aOs08q8/s3GueOjrMr9?="
		"=?GB2312?B?vt271ri0o6y158TUzqy7pKOstefE1New0N6jrMnPw8W3/s7xo6w=?=";
#else
	const char* s1 = "=?gb2312?B?sg==?="
		"=?gb2312?B?y8bX?=";
#endif
	rfc2047_test(rfc2047, s1);
	/////////////////////////////////////////////////////////////////////

#if 1
	const char* s2 = "=?gb2312?Q?=D0=EC=B8=D5=BB=D4 <xuganghui>?="
		"=?gb2312?B?LCDl0NKjz8ksIA==?==?utf-8?B?5Lit5Zu95Lq65rCR5YWx5ZKM5Zu9?="
		"=?BIG5?B?pKSw6qRIpcGmQKlNsOo=?="
		"=?gb2312?Q?=D0=EC=B8=D5=BB=D4 <xuganghui>?=";
#elif 1
	const char* s2 = "=?utf-8?B?5Lit5Zu95Lq65rCR5YWx5ZKM5Zu9?=";
#else
	const char* s2 = "=?BIG5?B?pKSw6qRIpcGmQKlNsOo=?="
		"=?gb2312?Q?=D0=EC=B8=D5=BB=D4 <xuganghui>?=";
#endif
	rfc2047_test(rfc2047, s2);
	rfc2047_test2(rfc2047, s2);

	/////////////////////////////////////////////////////////////////////

	const char* s3 = "=?UTF-8?B?55uR5o6n5a6d?= <report-noreply@jiankongbao.com>";
	rfc2047_test(rfc2047, s3);

	/////////////////////////////////////////////////////////////////////

	const char* s4 = "=?gb2312?B?ILG+uavLvs6qwKm088rQs6G+rdOq0qrV0tPQvq28w8q1waa1"
		"xL/Nu6fAtLmy0w==?==?GB2312?B?w7+qt6LK0LOhu/LWsb3TwvLC9Lj4v827pw==?=";
	rfc2047_test(rfc2047, s4);

	/////////////////////////////////////////////////////////////////////

	const char* s5 = "=?GB2312?B?zOzT7sb7w7M=?= <pu@163.com>";
	rfc2047_test(rfc2047, s5);

	/////////////////////////////////////////////////////////////////////
	
	printf("\r\n\r\n");
	printf("///////////////////////////////////////////////////////\r\n");

	const char* s6 = "=?utf-8?B?6LaF57qn5L2O5Lu377yB57ud5a+55aSn54mM77yB5LuFNDblhYPmiqLotK0=?=  =?utf-8?B?5paw54mI5YWw6JS75rC05Lu957yY6IiS57yT5p+U6IKk5ZWr5Zax5Lit5qC3?=  =?utf-8?B?77yBM+aKmOaKoui0rei2hee7j+WFuOmbhemhv+aZmuWuieWlveecoOa7iw==?=  =?utf-8?B?5YW76Zyc5ZCM5pyf6L+b6KGM77yB?=";
	rfc2047_test(rfc2047, s6);

	const char* s61 = "=?utf-8?B?6LaF57qn5L2O5Lu377yB57ud5a+55aSn54mM77yB5LuFNDblhYPmiqLotK0=?=";
	rfc2047_test(rfc2047, s61);

	const char* s62 = "=?utf-8?B?5paw54mI5YWw6JS75rC05Lu957yY6IiS57yT5p+U6IKk5ZWr5Zax5Lit5qC3?=";
	rfc2047_test(rfc2047, s62);

	const char* s63 = "=?utf-8?B?77yBM+aKmOaKoui0rei2hee7j+WFuOmbhemhv+aZmuWuieWlveecoOa7iw==?=";
	rfc2047_test(rfc2047, s63);
	
	const char* s64 = "=?utf-8?B?5YW76Zyc5ZCM5pyf6L+b6KGM77yB?=";
	rfc2047_test(rfc2047, s64);

	printf("///////////////////////////////////////////////////////\r\n");
	printf("\r\n\r\n");

	/////////////////////////////////////////////////////////////////////

	const char* s7 = "=?UTF-8?B?MjAxMeW5tOaYpeiKguelneemjw==?=";
	rfc2047_test(rfc2047, s7);

	/////////////////////////////////////////////////////////////////////

	const char* s8 = "=?GB18030?Q?=BE=B4=F6=A9_=D3=A6=C6=B8=D6=B0=CE=BB:_?==?GB18030?Q?=CF=FA=CA=DB=B4=FA=B1=ED_(201?==?GB18030?Q?1-05-31=C0=B4=D7=D401HR.COM)?=";
	rfc2047_test(rfc2047, s8);

	/////////////////////////////////////////////////////////////////////

	const char* s9 = "=?GB18030?Q?=B3=C2=B3=AC_=D3=A6=C6=B8=D6=B0=CE=BB:_J?=\r\n"
			" =?GB18030?Q?AVA=B8=DF=BC=B6=B9=A4=B3=CC=CA=A6_(?=\r\n"
			" =?GB18030?Q?2011-05-31=C0=B4=D7=D401HR.COM)?=\r\n";
	rfc2047_test(rfc2047, s9);

	/////////////////////////////////////////////////////////////////////

	const char* s10 = "Subject: =?utf-8?B?54ix576O5bCx5p2l77yB5LuFNTjlhYPvvIHkuqvljp/ku7cx?=\r\n"
		" =?utf-8?B?MTgw5YWD44CO5Lid55Ge57yHU1BB55Sf5rS76aaG44CP55qE576O5a65?=\r\n"
		" =?utf-8?B?576O5L2T5aWX6aSQ77yI5piO55y454m55pWI5oqk55CGLzUw5YiG6ZKf?=\r\n"
		" =?utf-8?B?K+iDjOmDqOS4ieeEpuiIkueptC81MOWIhumSnyvnvo7og7jkv53lgaXo?=\r\n"
		" =?utf-8?B?r77nqIsvNTDliIbpkp8r5paw5pWI5rC05ram5Lqu55m95oqk55CGLzgw?=\r\n"
		" =?utf-8?B?5YiG6ZKf77yJ77yM6K6p576O5Li95p2l55qE6LGq5peg5Y6L5Yqb77yB?=\r\n"
		" =?utf-8?B?\?=\r\n";

	rfc2047_test(rfc2047, s10);

	/////////////////////////////////////////////////////////////////////
	
	const char* s11 = "=?utf-8?B?57rnur/kuIrmg4XlhrXnu5/orqE=?=";
	rfc2047_test(rfc2047, s11);

	getchar();
	return (0);
}
