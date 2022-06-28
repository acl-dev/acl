#include "lib_acl.h"
#include <assert.h>

static void test1(void)
{
	char  buf[256];

	snprintf(buf, sizeof(buf), "hello world!");
	memmove(buf + 1, buf, strlen(buf) + 1);
	printf("buf:(%s)\n", buf);
}

int main(void)
{
	ACL_VSTREAM *fp = acl_vstream_fdopen(0, O_RDWR, 4096, 0, ACL_VSTREAM_TYPE_SOCK);
	char  buf[256];
	const char *data2 = "hello", *data1 = "world";
	int   ret;

	test1();

	(void) acl_vstream_unread(fp, data1, strlen(data1));
	acl_vstream_ungetc(fp, ' ');
	(void) acl_vstream_unread(fp, data2, strlen(data2));
	ret = acl_vstream_read(fp, buf, sizeof(buf));
	assert(ret > 0);
	buf[ret] = 0;
	printf("buf: %s, len: %d, %d\n", buf, ret, (int) strlen(buf));

	(void) acl_vstream_unread(fp, data1, strlen(data1));
	ret = acl_vstream_read(fp, buf, 2);
	assert(ret > 0);
	buf[ret] = 0;
	printf("buf: %s, len: %d\n", buf, ret);

	(void) acl_vstream_unread(fp, data2, strlen(data2));
	ret = acl_vstream_read(fp, buf, sizeof(buf));
	assert(ret > 0);
	buf[ret] = 0;
	printf("buf: %s, len: %d\n", buf, ret);

	acl_vstream_ungetc(fp, '>');
	acl_vstream_ungetc(fp, '^');
	acl_vstream_ungetc(fp, '<');
	ret = acl_vstream_read(fp, buf, sizeof(buf));
	assert(ret > 0);
	buf[ret] = 0;
	printf("buf: %s, len: %d\n", buf, ret);

	acl_vstream_fclose(fp);
	return (0);
}
