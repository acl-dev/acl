// vstream_fseek.cpp : 定义控制台应用程序的入口点。
//

#include "lib_acl.h"
#include <assert.h>

int main(int argc acl_unused, char *argv[] acl_unused)
{
	ACL_VSTREAM *fp;
	char  buf[4096];
	int   i, n;
	acl_off_t off, off2;
	struct acl_stat sbuf;

	fp = acl_vstream_fopen("test.txt", O_RDWR | O_CREAT, 0600, 4096);
	assert(fp);

	if (acl_fstat(ACL_VSTREAM_FILE(fp), &sbuf) < 0)
		printf("fstat file error\r\n");
	else
		printf("fstat file's size=%d\r\n", (int) sbuf.st_size);

	memset(buf, 'X', sizeof(buf));

	for (i = 0; i < 10; i++) {
		acl_vstream_writen(fp, buf, sizeof(buf));
		printf("after write %d, offset=%d, sys_offset=%d\r\n",
			(int) sizeof(buf), (int) fp->offset, (int) fp->sys_offset);
	}

	n = acl_vstream_read(fp, buf, sizeof(buf) - 1);
	if (n == ACL_VSTREAM_EOF)
		printf("ok(%d): read ACL_VSTREAM_EOF, offset=%d, sys_offset=%d\r\n",
			__LINE__, (int) fp->offset, (int) fp->sys_offset);
	else {
		printf("error(%d): read n=%d\r\n", __LINE__, n);
		goto END;
	}

	off = acl_vstream_fseek(fp, 0, SEEK_SET);
	if (off == -1) {
		printf("error(%d): fseek error\r\n", __LINE__);
		goto END;
	}
	printf("ok(%d): after seek 0, offset=%d, sys_offset=%d\r\n",
		__LINE__, (int) fp->offset, (int) fp->sys_offset);

	n = acl_vstream_read(fp, buf, sizeof(buf));
	if (n == ACL_VSTREAM_EOF) {
		printf("error(%d): readr\r\n", __LINE__);
		goto END;
	}
	printf("ok(%d): after read %d, offset=%d, sys_offset=%d\r\n",
		__LINE__, (int) sizeof(buf), (int) fp->offset, (int) fp->sys_offset);

	off = acl_vstream_fseek(fp, 100, SEEK_SET);
	if (off != 100) {
		printf("error(%d): off(%d) != 100\r\n", __LINE__, (int) off);
		goto END;
	} else
		printf("ok(%d): after seek 100, offset=%d, sys_offset=%d\r\n",
			__LINE__, (int) fp->offset, (int) fp->sys_offset);
	off2 =  acl_vstream_ftell(fp);
	if (off2 != off) {
		printf("error, off: %lld != ftell(%lld)\r\n", off, off2);
		goto END;
	} else
		printf(">>>ftell: %lld ok\r\n", off2);

	off = acl_vstream_fseek(fp, 4096, SEEK_SET);
	if (off != 4096) {
		printf("error(%d): off(%d) != 4096\r\n", __LINE__, (int) off);
		goto END;
	} else
		printf("ok(%d): after seek 4096, offset=%d, sys_offset=%d\r\n",
			__LINE__, (int) fp->offset, (int) fp->sys_offset);

	off = acl_vstream_fseek(fp, 5096, SEEK_SET);
	if (off != 5096) {
		printf("error(%d): off(%d) != 5096\r\n", __LINE__, (int) off);
		goto END;
	} else
		printf("ok(%d): after seek 5096, offset=%d, sys_offset=%d\r\n",
			__LINE__, (int) fp->offset, (int) fp->sys_offset);

	off = acl_vstream_fseek(fp, 8191, SEEK_SET);
	if (off != 8191) {
		printf("error(%d): off(%d) != 8191\r\n", __LINE__, (int) off);
		goto END;
	} else
		printf("ok(%d): after seek 8191, offset=%d, sys_offset=%d\r\n",
			__LINE__, (int) fp->offset, (int) fp->sys_offset);

	off = acl_vstream_fseek(fp, 8192, SEEK_SET);
	if (off != 8192) {
		printf("error(%d): off(%d) != 8192\r\n", __LINE__, (int) off);
		goto END;
	} else
		printf("ok(%d): after seek 8192, offset=%d, sys_offset=%d\r\n",
			__LINE__, (int) fp->offset, (int) fp->sys_offset);

	off = acl_vstream_fseek(fp, 8193, SEEK_SET);
	if (off != 8193) {
		printf("error(%d): off(%d) != 8193\r\n", __LINE__, (int) off);
		goto END;
	} else
		printf("ok(%d): after seek 8193, offset=%d, sys_offset=%d\r\n",
			__LINE__, (int) fp->offset, (int) fp->sys_offset);

	// 系列读
	printf("%d>>>begin check: offset=%d, sys_offset=%d\r\n",
		__LINE__, (int) fp->offset, (int) fp->sys_offset);
	off = acl_vstream_fseek(fp, 100, SEEK_SET);
	if (off != 100) {
		printf("error(%d): off(%d) != 100\r\n", __LINE__, (int) off);
		goto END;
	} else
		printf("ok(%d): after seek 100, offset=%d, sys_offset=%d\r\n",
			__LINE__, (int) fp->offset, (int) fp->sys_offset);
	acl_vstream_read(fp, buf, 100);
	printf("ok(%d): after read 100, offset=%d, sys_offset=%d\r\n",
		__LINE__, (int) fp->offset, (int) fp->sys_offset);

	off = acl_vstream_fseek(fp, 100, SEEK_SET);
	if (off != 100) {
		printf("error(%d): off(%d) != 100\r\n", __LINE__, (int) off);
		goto END;
	} else
		printf("ok(%d): after seek 100, offset=%d, sys_offset=%d\r\n",
			__LINE__, (int) fp->offset, (int) fp->sys_offset);

	off = acl_vstream_fseek(fp, 200, SEEK_SET);
	if (off != 200) {
		printf("error(%d): off(%d) != 200\r\n", __LINE__, (int) off);
		goto END;
	} else printf("ok(%d): after seek 200, offset=%d, sys_offset=%d\r\n",
			__LINE__, (int) fp->offset, (int) fp->sys_offset);
	acl_vstream_read(fp, buf, 100);
	printf("ok(%d): after read 100, offset=%d, sys_offset=%d\r\n",
		__LINE__, (int) fp->offset, (int) fp->sys_offset);

	off = acl_vstream_fseek(fp, 300, SEEK_SET);
	if (off != 300) {
		printf("error(%d): off(%d) != 300\r\n", __LINE__, (int) off);
		goto END;
	} else
		printf("ok(%d): after seek 300, offset=%d, sys_offset=%d\r\n",
			__LINE__, (int) fp->offset, (int) fp->sys_offset);
	acl_vstream_read(fp, buf, 10);
	printf("ok(%d): after read 10, offset=%d, sys_offset=%d\r\n",
		__LINE__, (int) fp->offset, (int) fp->sys_offset);

	acl_vstream_writen(fp, "hello", strlen("hello"));
	printf("ok(%d): after write 'hello', offset=%d, sys_offset=%d\r\n",
		__LINE__, (int) fp->offset, (int) fp->sys_offset);
	acl_vstream_writen(fp, "world", strlen("world"));
	printf("ok(%d): after write 'world', offset=%d, sys_offset=%d\r\n",
		__LINE__, (int) fp->offset, (int) fp->sys_offset);

	off = acl_vstream_fseek(fp, 8190, SEEK_SET);
	if (off != 8190) {
		printf("error(%d): off(%d) != 8190\r\n", __LINE__, (int) off);
		goto END;
	} else
		printf("ok(%d): after seek 8190, offset=%d, sys_offset=%d\r\n",
			__LINE__, (int) fp->offset, (int) fp->sys_offset);

	off2 = acl_vstream_ftell(fp);
	if (off2 != off) {
		printf("ftell(%lld) != off(%lld), error\r\n", off2, off);
		goto END;
	} else
		printf("ftell(%lld) == off(%lld), ok\r\n", off2, off);

	n = acl_vstream_read(fp, buf, 100);
	printf("ok(%d): after read 100, n=%d offset=%d, sys_offset=%d\r\n",
		__LINE__, n, (int) fp->offset, (int) fp->sys_offset);

END:
	acl_vstream_close(fp);
	getchar();

	return 0;
}

