// md5.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <assert.h>
#include "acl_cpp/stdlib/md5.hpp"
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

#ifdef WIN32
	getchar();
#endif
	return 0;
}
