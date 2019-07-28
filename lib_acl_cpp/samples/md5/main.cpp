// md5.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <assert.h>
#include "md5_c.h"

int main()
{
	const char* s = "中国人民银行！";
	const char* key = "zsxxsz";
	char  buf1[33], buf2[33];

	acl::md5::md5_string(s, strlen(s), key, strlen(key),
		buf1, sizeof(buf1));
	printf("first md5: %s\r\n", buf1);

	md5_string(s, key, strlen(key), buf2, sizeof(buf2));
	printf("second md5: %s\r\n", buf2);

	assert(strcmp(buf1, buf2) == 0);

	/////////////////////////////////////////////////////////////////////

	acl::md5 md5;

	s = "ABCDEFGHIGKLMNOPQRSTUVWXYZ";
	md5.update(s, strlen(s));
	md5.finish();
	acl::safe_snprintf(buf1, sizeof(buf1), "%s", md5.get_string());

	md5.reset();

	char ch;
	size_t len = strlen(s);
	for (size_t i = 0; i < len; i++)
	{
		ch = s[i];
		md5.update(&ch, 1);
	}
	md5.finish();
	acl::safe_snprintf(buf2, sizeof(buf2), "%s", md5.get_string());

	if (strcmp(buf1, buf2) == 0)
		printf("OK: %s\r\n", buf1);
	else
		printf("error, buf1: %s, buf2: %s\r\n", buf1, buf2);

	/////////////////////////////////////////////////////////////////////

	md5.reset();
	len = 1024000;
	char* buf = (char*) malloc(len);

	for (size_t i = 0; i < len; i++)
	{
		ch = i % 255;
		buf[i] = ch;
		md5.update(&ch, 1);
	}
	md5.finish();
	acl::safe_snprintf(buf1, sizeof(buf1), "%s", md5.get_string());

	md5.reset();
	md5.update(buf, len);
	md5.finish();
	acl::safe_snprintf(buf2, sizeof(buf2), "%s", md5.get_string());

	if (strcmp(buf1, buf2) == 0)
		printf("OK2: %s\r\n", buf1);
	else
		printf("error, buf1: %s, buf2: %s\r\n", buf1, buf2);
#ifdef WIN32
	getchar();
#endif
	return 0;
}
