/* #include "StdAfx.h"*/
#ifndef ACL_PREPARE_COMPILE

#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>

#endif

#include "test_stdlib.h"

int test_strrncasecmp(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	const char *s01 = "hello WorLd", *s02 = "world";
	const char *s11 = "hello", *s12 = "world";

	acl_assert(strrncasecmp(s01, s02, strlen(s02)) == 0);
	acl_assert(strrncasecmp(s01, s02, strlen(s02) + 1) != 0);
	acl_assert(strrncasecmp(s11, s12, strlen(s12)) != 0);

	return (0);
}

int test_strrncmp(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	const char *s01 = "hello world", *s02 = "world";
	const char *s11 = "hello", *s12 = "world";

	acl_assert(strrncmp(s01, s02, strlen(s02)) == 0);
	acl_assert(strrncmp(s01, s02, strlen(s02) + 1) != 0);
	acl_assert(strrncmp(s11, s12, strlen(s12)) != 0);

	return (0);
}

int test_x64toa(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	char  buf[32], buf2[32];
	acl_uint64 n = -1;
	int   max = 10240000, i;

	acl_ui64toa_radix(n, buf, sizeof(buf), 10);
	acl_ui64toa(n, buf2, sizeof(buf2));
	printf("buf: %s, len: %d; buf2: %s, len: %d; %llu\n",
		buf, (int) strlen(buf), buf2, (int) strlen(buf2), (acl_uint64) (-1));

	ACL_METER_TIME("use acl_uint64toa_radix begin");
	for (i = 0; i < max; i++)
		acl_ui64toa_radix(n, buf, sizeof(buf), 10);
	ACL_METER_TIME("use acl_uint64toa_radix end");

	ACL_METER_TIME("use acl_uint64toa begin");
	for (i = 0; i < max; i++)
		acl_ui64toa(n, buf, sizeof(buf));
	ACL_METER_TIME("use acl_uint64toa end");

	return (0);
}

int test_strcasestr(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	char *src = "href=\"http://3g.hexun.com\" target=\"_blank\" style=\"background:url(http://www.hexun.com/upload/phaone2.png) no-repeat; padding:2px 0 2px 13px;\">ÊÖ»ú°æ</a></div>";
	const char *ptr = ">", *ptr1;

	ptr1 = acl_strcasestr(src, ptr);
	if (ptr1 == NULL) {
		printf(">>>error, not found %s from %s\n", ptr, src);
		return (-1);
	}
	printf(">>>ok, find it(%s): %s\n", ptr, ptr1);

	ptr = "http://";
	ptr1 = acl_strcasestr(src, ptr);
	if (ptr1 == NULL) {
		printf(">>>error, not found %s from %s\n", ptr, src);
		return (-1);
	}
	printf(">>>ok, find it(%s): %s\n", ptr, ptr1);

	return (0);
}

