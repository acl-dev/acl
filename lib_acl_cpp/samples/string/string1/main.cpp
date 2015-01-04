#include "lib_acl.h"
#include "acl_cpp/stream/ofstream.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include <stdio.h>
#include <string>

// std::string ²âÊÔ

static void test(void)
{
	std::string s;

	printf("--------------------- in test -----------------\r\n");

	s = "XXX";
	printf(">>s: %s, len: %d\n", s.c_str(), (int) s.length());

	acl::string s1("hello world, You're welcome!");
	printf("strip: '\t !', before strip: %s, ", s1.c_str());
	s1.strip("\t !", true);
	printf("after strip: %s\r\n", s1.c_str());
	acl_assert(s1 == "helloworld,You'rewelcome");

	s1 = "hello world, You're welcome!";
	s1.strip(":", true);
	printf(">>s1: %s\n", s1.c_str());
	acl_assert(s1 == "hello world, You're welcome!");

	s1 = "hello world, You're welcome!";
	s1.strip("he", true);
	printf(">>s1: %s\n", s1.c_str());
	acl_assert(s1 == "llo world, You'r wlcom!");

	s1 = "hello world, You're welcome!";
	s1.strip("he");
	printf(">>s1: %s\n", s1.c_str());
	acl_assert(s1 == "llo world, You're welcome!");

	s1 = "hello world, You're welcome!";
	s1.strip("he ", true);
	printf(">>s1: %s\n", s1.c_str());
	acl_assert(s1 == "lloworld,You'rwlcom!");
}

static void test2(void)
{
	acl::string s, s1("hello1;"), s2("hello2;");

	printf("------------------- in test2 ----------------------\r\n");
	s.append("<");
	s << s1;
	s << s2;
	s.append(">");
	printf(">>>%s\n", s.c_str());
}

static void test3(void)
{
	printf("------------------- in test3 ------------------\r\n");
	printf("parse_int: %s\n", acl::string::parse_int(100).c_str());
	printf("parse_int(-1): %s, %s\n",
		acl::string::parse_int(-1).c_str(),
		acl::string::parse_int((unsigned int) -1).c_str());
	printf("parse_int64(-1): %s, %s\n",
		acl::string::parse_int(-1).c_str(),
		acl::string::parse_int64((unsigned long long int) -1).c_str());

	unsigned char n1 = 255;
	char n2 = (char) 255;

	printf("parse_int: unsigned char %s, char %s\n",
		acl::string::parse_int(n1).c_str(),
		acl::string::parse_int(n2).c_str());
}

//////////////////////////////////////////////////////////////////////////

static void test41(void)
{
	acl::string buf;
	buf = "hello world!";

	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";

	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";

	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";

	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
}

// Ñ¹Á¦²âÊÔ acl::string

static void test4(void)
{
	ACL_METER_TIME(">> begin acl::string");
	for (int i = 0; i < 100000; i++)
	{
		test41();
	}
	ACL_METER_TIME(">> end acl::string");
}

//////////////////////////////////////////////////////////////////////////

static void test51(void)
{
	std::string buf;
	buf = "hello world!";

	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";

	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";

	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";

	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
}

// Ñ¹Á¦²âÊÔ std::string

static void test5(void)
{
	ACL_METER_TIME(">> begin std::string");
	for (int i = 0; i < 100000; i++)
	{
		test51();
	}
	ACL_METER_TIME(">> end std::string");
}

//////////////////////////////////////////////////////////////////////////

static void test61(void)
{
	ACL_VSTRING* buf;
	buf = acl_vstring_alloc(256);

	acl_vstring_strcpy(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");

	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");

	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");

	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");

	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");
	acl_vstring_strcat(buf, "hello world!");

	acl_vstring_free(buf);
}

static void test6(void)
{
	ACL_METER_TIME(">> begin ACL_VSTRING");
	for (int i = 0; i < 100000; i++)
	{
		test61();
	}
	ACL_METER_TIME(">> end ACL_VSTRING");
}

//////////////////////////////////////////////////////////////////////////

static void test71(void)
{
	ACL_VSTRING* buf = acl_vstring_alloc(256);

	acl_vstring_sprintf(buf, "max unsigned int: %u; max unsigned long long int: %llu",
		(unsigned int) -1, (unsigned long long) -1);
	acl_vstring_free(buf);
}

static void test7(void)
{
	ACL_METER_TIME(">> begin ACL_VSTRING: acl_vstring_sprintf");
	for (int i = 0; i < 100000; i++)
	{
		test71();
	}
	ACL_METER_TIME(">> end ACL_VSTRING: acl_vstring_sprintf");
}

//////////////////////////////////////////////////////////////////////////

static void test_main(void)
{
	acl::string s;
	int  n = 200;
	unsigned char ch;

	printf("-------------------- in test_main ---------------\r\n");

	//////////////////////////////////////////////////////////////////////

	s += (char) n;
	ch = s[0];
	n = (unsigned char) s[0];
	printf("n: %d, ch: %d\n", n, ch);

	//////////////////////////////////////////////////////////////////////

	s.clear();
	s.format("\tunsigned long long -1: %llu\r\n"
		"\tunsigned int -1: %u\r\n"
		"\tunsigned short -1: %u\r\n"
		"\tunsigned char -1: %u\r\n",
		(unsigned long long) -1, (unsigned int) -1,
		(unsigned short) -1, (unsigned char) -1);
	printf(">>> 0 result:\r\n");
	printf("%s", s.c_str());

	s.clear();
	s<<"append char: " << 'X';
	printf(">>> 1 result: %s\r\n", s.c_str());

	s = 'X';
	printf(">>> 2 result: %s\r\n", s.c_str());

	//////////////////////////////////////////////////////////////////////
	s.clear();
	printf(">>> set bin status\r\n");
	s.set_bin(true);
#ifdef	WIN32
	s << (unsigned __int64) -1 << (unsigned int) -1 << (unsigned short) -1 << (unsigned char) 'Z';
#else
	s << (unsigned long long int) -1 << (unsigned int) -1 << (unsigned short) -1 << (unsigned char) 'Z';
#endif
	printf(">>> 3 result: %s\r\n", s.c_str());

#ifdef	WIN32
	unsigned __int64 n1;
#else
	unsigned long long int n1;
#endif
	unsigned int n2;
	unsigned short n3;
	unsigned char n4;
	s >> n1;
	s >> n2;
	s >> n3;
	s >> n4;

	s.format("\tunsigned int64 -1: %llu\r\n"
		"\tunsigned int -1: %u\r\n"
		"\tunsigned short -1: %u\r\n"
		"\tunsigned char -1: %u, %c\r\n",
		n1, n2, n3, n4, n4);
	printf(">>> 4 result: \r\n%s\r\n", s.c_str());

	//////////////////////////////////////////////////////////////////////
	s.clear();
	s.set_bin(false);
	printf(">>> set asc status\r\n");
#ifdef	WIN32
	s << (unsigned __int64) -1 << ", "
		<< (unsigned int) -1 << ", "
		<< (unsigned short) -1 << ", "
		<< (unsigned char) 'Z';
#else
	s << (unsigned long long int) -1 << ", "
		<< (unsigned int) -1 << ", "
		<< (unsigned short) -1 << ", "
		<< (unsigned char) 'Z';
#endif
	printf(">>> 5 result: %s\r\n", s.c_str());

	//////////////////////////////////////////////////////////////////////

	printf("\r\n");

	test();
	test2();
	test3();
}

static void test31(void)
{
	char buf[1];
	const char* s = "01234567890123456789";
	int ret = acl::safe_snprintf(buf, sizeof(buf), "%s", s);
	printf(">>ret: %d, buf: %s, len: %d\r\n", ret, buf, (int) strlen(buf));
}

int main(void)
{
	//acl_mem_slice_init(8, 1024, 1000000, ACL_SLICE_FLAG_GC2 | ACL_SLICE_FLAG_RTGC_OFF);
	test31();
	getchar();

	acl::string buf;
	const char* sss = "hello world!";
	buf.format("max unsigned short: %30u\r\n", (unsigned short) -1);
	buf.format_append("max unsigned int: %30u\r\n", (unsigned int) -1);
	buf.format_append("max unsigned long long int: %30llu\r\n",
		(unsigned long long int) -1);
	buf.format_append("sss: %s, max unsigned int: %u", sss,
		(unsigned int) -1);
	printf(">>buf: %s\r\n", buf.c_str());
	printf("enter any key to continue\r\n");
	getchar();

	const char* s[] = { "a.b", "a.", ".b", ".", "ab", "abcd.txt", NULL };
	acl::string s1, s2, ss;
	for (size_t i = 0;  s[i] != NULL; i++)
	{
		ss = s[i];
		const char* p = strchr(ss.c_str(), '.');
		if (p)
		{
			printf("----------------------------------------------------\r\n");
			int n = (int) (p - ss.c_str());
			printf("string: |%s|, pos: %d\r\n", ss.c_str(), n);

			s1 = ss.left(n);
			printf("left: |%s|, len: %d\r\n", s1.c_str(), (int) s1.length());

			s2 = ss.right(n);
			printf("right: |%s|, len: %d\r\n", s2.c_str(), (int) s2.length());
		}
	}
	printf("enter any key to continue ...\r\n");
	getchar();

	test_main();

	if (1)
	{
		test4();
		test5();
		test6();
		test7();
	}

	acl::string sss2("hello|world,;hello1|waaaa;sdsdsdsdds|");
	const std::vector<acl::string>& tokens = sss2.split2(";,|");
	std::vector<acl::string>::const_iterator cit = tokens.begin();
	for (; cit != tokens.end(); ++cit)
	{
		printf("%s\r\n", (*cit).c_str());
	}
#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return (0);
}
