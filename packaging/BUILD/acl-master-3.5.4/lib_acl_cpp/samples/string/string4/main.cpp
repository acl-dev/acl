#include "lib_acl.h"
#include <vector>
#include <iostream>
#include "acl_cpp/lib_acl.hpp"
#include <stdio.h>
#include <string>

static void test_strip(int lineno, const char* src, const char* needle,
	const char* result, bool each = true)
{
	acl::string buf(src);
	buf.strip(needle, each);

	printf(">> each: %s, len: %d, src: >%s<\r\n",
		each ? "yes" : "no", (int) strlen(src), src);
	printf(">> len: %d, buf: >%s<\r\n",
		(int) buf.size(), buf.c_str());

	if (buf == result)
		printf(">> line: %d, OK!\r\n", lineno);
	else
	{
		printf(">> line: %d, Error!\r\n", lineno);
		exit (1);
	}

	printf("\r\nEnter any key to continue ...");
	fflush(stdout);
	getchar();
}

int main(int argc, char* argv[])
{
	acl::string buf;
	buf = acl::string("");
	printf("buf: %s, size: %d\r\n", buf.c_str(), (int) buf.length());

	buf = 1000;
	printf("buf: %s, size: %d\r\n", buf.c_str(), (int) buf.length());

	ACL_VSTRING* s1 = acl_vstring_alloc(100);
	acl_vstring_memcpy(s1, "", 0);
	printf("s1: %s\r\n", acl_vstring_str(s1));
	acl_vstring_free(s1);

	acl::rfc822 rfc;
	long t = 1437362621;
	char tt[128];
	if (argc >= 2)
		t = atol(argv[1]);
	rfc.mkdate(t, tt, sizeof(tt));
	printf(">>>t: %s\r\n", tt);

	printf("Enter any key to continue ...");
	fflush(stdout);
	getchar();

	test_strip(__LINE__, "hello world!", "\r\n", "hello world!");
	test_strip(__LINE__, "\r\n", "\r\n", "");
	test_strip(__LINE__, ".\r\n", "\r\n", ".");
	test_strip(__LINE__, "hello\r\n", "\r\n", "hello");
	test_strip(__LINE__, "\r\nhello\r\n world\r\n zsx\n xsz\r\n zsxxsz\n\n",
		"\r\n", "hello world zsx xsz zsxxsz");
	test_strip(__LINE__, "\r\nhello\r\n world\r\n zsx\n xsz\r\n zsxxsz\n\n",
		" \r\n", "helloworldzsxxszzsxxsz");
	test_strip(__LINE__, "\r\nhello\r\n world\r\n zsx\n xsz\r\n zsxxsz\n\n",
		"l\r\n", "heo word zsx xsz zsxxsz");
	test_strip(__LINE__, "\r\nhello\r\n world\r\n zsx\n xsz\r\n zsxxsz\n\n",
		" l\r\n", "heowordzsxxszzsxxsz");

	test_strip(__LINE__, "\r\nhello\r\n world\r\n zsx\r\n xsz\r\n zsxxsz\r\n\r\n",
		"\r\n", "hello world zsx xsz zsxxsz", false);
	test_strip(__LINE__, "\r\nhello\r\n world\r\n zsx\n xsz\r\n zsxxsz\n\n",
		"\r\n", "hello world zsx\n xsz zsxxsz\n\n", false);
	test_strip(__LINE__, "hello\r\n world\r\n you're\r\n welcome!\r\n\r\n",
		"\r\n", "hello world you're welcome!", false);
	test_strip(__LINE__, "hello\n world\r\n you're\n welcome!\r\n\r\n",
		"\r\n", "hello\n world you're\n welcome!", false);
	test_strip(__LINE__, "\r\n\r\nhello\n world\r\n you're\n welcome!",
		"\r\n", "hello\n world you're\n welcome!", false);
	test_strip(__LINE__, "\n\r\nhello\n world\r\n you're\n welcome!",
		"\r\n", "\nhello\n world you're\n welcome!", false);

	return 0;
}
