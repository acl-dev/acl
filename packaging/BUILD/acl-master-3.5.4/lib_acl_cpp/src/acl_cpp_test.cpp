#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/acl_cpp_test.hpp"

namespace acl
{

void test_logger(void)
{
	logger("logger ok!");
	logger_warn("logger_warn ok!");
	logger_error("logger_error ok!");
	logger_fatal("logger_fatal ok!");
}

void test_snprintf(void)
{
	char buf[10];
	const char* s9  = "xxxxxxxxx";
	const char* s10 = "xxxxxxxxxx";
	const char* s11 = "xxxxxxxxxxx";
	int ret;
	
	memset(buf, 0, sizeof(buf));
	ret = safe_snprintf(buf, sizeof(buf), "%s", s9);
	printf("buf size: %d, string len: %d, ret: %d, buf: %s\r\n",
		(int) sizeof(buf), (int) strlen(s9), ret, buf);

	memset(buf, 0, sizeof(buf));
	ret = safe_snprintf(buf, sizeof(buf), "%s", s10);
	printf("buf size: %d, string len: %d, ret: %d, buf: %s\r\n",
		(int) sizeof(buf), (int) strlen(s10), ret, buf);

	memset(buf, 0, sizeof(buf));
	ret = safe_snprintf(buf, sizeof(buf), "%s", s11);
	printf("buf size: %d, string len: %d, ret: %d, buf: %s\r\n",
		(int) sizeof(buf), (int) strlen(s11), ret, buf);
}

}
