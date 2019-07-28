
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "lib_acl.h"

static void test_fseek3(int buflen)
{
	ACL_VSTREAM *fp;
	ACL_VSTRING *why = acl_vstring_alloc(100);
	char  filename[] ="./tmp.txt";
	char  buf[10];
	int   n;
	int offset, off2;
	int   ch;

	fp = acl_safe_open(filename, O_CREAT | O_RDWR, 0600,
			(struct stat *) 0, (uid_t)-1,
			(uid_t )-1, why);
	if (fp == 0)
		acl_msg_fatal("%s(%d)->%s: can't open %s, serr = %s",
			__FUNCTION__, __LINE__, __FILE__,
			filename, acl_vstring_str(why));

	fp->read_buf_len = buflen;

	ch = acl_vstream_getc(fp);
	printf("after read(1) >>> ch = %c, read_cnt = %d, offset=%d, %d\n",
		ch, (int) fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);
	offset = acl_vstream_fseek(fp, 0, SEEK_CUR);
	printf("after fseek(0) >>> ch = %c, read_cnt = %d, off = %d, offset=%d, %d\n\n",
		ch, (int) fp->read_cnt, offset, (int) fp->offset, (int) fp->sys_offset);

	ch = acl_vstream_getc(fp);
	printf("after read(1) >>> ch = %c, read_cnt = %d, offset=%d, %d\n",
		ch, (int) fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);
	offset = acl_vstream_fseek(fp, 0, SEEK_CUR);
	printf("after fseek(0) >>> ch = %c, read_cnt = %d, off = %d, %s, offset=%d, %d\n\n",
		ch, (int) fp->read_cnt, offset, strerror(errno), (int) fp->offset, (int) fp->sys_offset);

	offset = acl_vstream_fseek(fp, 10, SEEK_CUR);
	off2 = lseek(ACL_VSTREAM_FILE(fp), 0, SEEK_CUR);
	printf("after fseek(10) >>> off=%d, lseek = %d, read_cnt = %d, offset=%d, %d\n\n",
		offset, off2, (int) fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);

	n = acl_vstream_gets_nonl(fp, buf, sizeof(buf) - 1);
	printf("after read(getline) >>> buf = [%s], fp->read_cnt = %d, offset=%d, %d\n",
		buf, (int) fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);
	offset = acl_vstream_fseek(fp, 0, SEEK_CUR);
	printf("after fseek(0) >>> buf = [%s], read_cnt = %d, off = %d, offset=%d, %d\n\n",
		buf, (int) fp->read_cnt, offset, (int) fp->offset, (int) fp->sys_offset);

	ch = acl_vstream_getc(fp);
	printf("after read(1) >>> ch = %c, read_cnt = %d, offset=%d, %d\n",
		ch, (int) fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);
	offset = acl_vstream_fseek(fp, 0, SEEK_CUR);
	printf("after fseek(0) >>> ch = %c, read_cnt = %d, off = %d, offset=%d, %d\n\n",
		ch, (int)fp->read_cnt, offset, (int) fp->offset, (int) fp->sys_offset);

	n = acl_vstream_readn(fp, buf, 2);
	buf[n] = 0;
	printf("after read(buf) >>> buf = [%s], fp->read_cnt = %d, off=%d, offset=%d, %d\n",
		buf, (int) fp->read_cnt, offset, (int) fp->offset, (int) fp->sys_offset);
	offset = acl_vstream_fseek(fp, 10, SEEK_CUR);
	printf("after fseek(10) >>> read_cnt = %d, off = %d, offset=%d, %d\n\n",
		(int) fp->read_cnt, offset, (int) fp->offset, (int) fp->sys_offset);

	ch = acl_vstream_getc(fp);
	printf("after read(1) >>> ch = %c, read_cnt = %d, offset=%d, %d\n",
		ch, (int) fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);
	offset = acl_vstream_fseek(fp, 0, SEEK_CUR);
	printf("after fseek(0) >>> ch = %c, read_cnt = %d, off = %d, %s, offset=%d, %d\n\n",
		ch, (int) fp->read_cnt, offset, strerror(errno), (int) fp->offset, (int) fp->sys_offset);

	acl_vstring_free(why);
	acl_vstream_close(fp);
}

static void test_fseek2(int buflen)
{
	ACL_VSTREAM *fp;
	ACL_VSTRING *why = acl_vstring_alloc(100);
	char  filename[] ="./tmp.txt";
	char  buf[10];
	int   n;
	int offset, off2;
	int   ch;

	fp = acl_safe_open(filename, O_CREAT | O_RDWR, 0600,
			(struct stat *) 0, (uid_t)-1,
			(uid_t )-1, why);
	if (fp == 0)
		acl_msg_fatal("%s(%d)->%s: can't open %s, serr = %s",
			__FILE__, __LINE__, __FUNCTION__, filename, acl_vstring_str(why));

	fp->read_buf_len = buflen;

	ch = acl_vstream_getc(fp);
	printf("after read(1)(%d) >>> ch = %c, read_cnt = %d, offset=%d, %d\n",
		__LINE__, ch, (int) fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);
	offset = acl_vstream_fseek2(fp, 0, SEEK_CUR);
	printf("after fseek(0)(%d) >>> ch = %c, read_cnt = %d, off = %d, offset=%d, %d\n\n",
		__LINE__, ch, (int)fp->read_cnt, offset, (int) fp->offset, (int) fp->sys_offset);

	ch = acl_vstream_getc(fp);
	printf("after read(1)(%d) >>> ch = %c, read_cnt = %d, offset=%d, %d\n",
		__LINE__, ch, (int) fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);
	offset = acl_vstream_fseek2(fp, 0, SEEK_CUR);
	printf("after fseek(0)(%d) >>> ch = %c, read_cnt = %d, off = %d, %s, offset=%d, %d\n\n",
		__LINE__, ch, (int)fp->read_cnt, offset, strerror(errno), (int) fp->offset, (int) fp->sys_offset);

	offset = acl_vstream_fseek2(fp, 10, SEEK_CUR);
	off2 = lseek(ACL_VSTREAM_FILE(fp), 0, SEEK_CUR);
	printf("after fseek(10)(%d) >>> off=%d, lseek = %d, read_cnt = %d, offset=%d, %d\n\n",
		__LINE__, offset, off2, (int)fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);

	n = acl_vstream_gets_nonl(fp, buf, sizeof(buf) - 1);
	printf("after read(getline)(%d) >>> buf = [%s], fp->read_cnt = %d, offset=%d, %d\n",
		__LINE__, buf, (int)fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);
	offset = acl_vstream_fseek2(fp, 0, SEEK_CUR);
	printf("after fseek(0)(%d) >>> buf = [%s], read_cnt = %d, off = %d, offset=%d, %d\n\n",
		__LINE__, buf, (int) fp->read_cnt, offset, (int) fp->offset, (int) fp->sys_offset);

	ch = acl_vstream_getc(fp);
	printf("after read(1)(%d) >>> ch = %c, read_cnt = %d, offset=%d, %d\n",
		__LINE__, ch, (int)fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);
	offset = acl_vstream_fseek2(fp, 0, SEEK_CUR);
	printf("after fseek(0)(%d) >>> ch = %c, read_cnt = %d, off = %d, offset=%d, %d\n\n",
		__LINE__, ch, (int) fp->read_cnt, offset, (int) fp->offset, (int) fp->sys_offset);

	n = acl_vstream_readn(fp, buf, 2);
	buf[n] = 0;
	printf("after read(buf)(%d) >>> buf = [%s], fp->read_cnt = %d, offset=%d, offset=%d, %d\n",
		__LINE__, buf, (int) fp->read_cnt, offset, (int) fp->offset, (int) fp->sys_offset);
	offset = acl_vstream_fseek2(fp, 10, SEEK_CUR);
	printf("after fseek(10)(%d) >>> read_cnt = %d, off = %d, offset=%d, %d\n\n",
		__LINE__, (int) fp->read_cnt, offset, (int) fp->offset, (int) fp->sys_offset);

	ch = acl_vstream_getc(fp);
	printf("after read(1)(%d) >>> ch = %c, read_cnt = %d, offset=%d, %d\n",
		__LINE__, ch, (int)fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);
	offset = acl_vstream_fseek2(fp, 0, SEEK_CUR);
	printf("after fseek(0)(%d) >>> ch = %c, read_cnt = %d, off = %d, %s, offset=%d, %d\n\n",
		__LINE__, ch, (int) fp->read_cnt, offset, strerror(errno), (int) fp->offset, (int) fp->sys_offset);

	acl_vstring_free(why);
	acl_vstream_close(fp);
}

static void test_fseek(int buflen)
{
	ACL_VSTREAM *fp;
	ACL_VSTRING *why = acl_vstring_alloc(100);
	char  filename[] ="./tmp.txt";
	char  buf[10];
	int   n;
	int offset, off2;
	int  ch;

	fp = acl_safe_open(filename, O_CREAT | O_RDWR, 0600,
			(struct stat *) 0, (uid_t)-1,
			(uid_t )-1, why);
	if (fp == 0)
		acl_msg_fatal("%s(%d)->%s: can't open %s, serr = %s",
			__FILE__, __LINE__, __FUNCTION__, filename, acl_vstring_str(why));

	fp->read_buf_len = buflen;

	ch = acl_vstream_getc(fp);
	printf("after read(1)(%d) >>> ch = %c, read_cnt = %d, offset=%d, %d\n",
		__LINE__, ch, (int) fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);
	offset = acl_vstream_fseek(fp, 0, SEEK_CUR);
	printf("after fseek(0)(%d) >>> ch = %c, read_cnt = %d, off = %d, offset=%d, %d\n\n",
		__LINE__, ch, (int) fp->read_cnt, offset, (int) fp->offset, (int) fp->sys_offset);

	ch = acl_vstream_getc(fp);
	printf("after read(1)(%d) >>> ch = %c, read_cnt = %d, offset=%d, %d\n",
		__LINE__, ch, (int) fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);
	offset = acl_vstream_fseek(fp, 0, SEEK_CUR);
	printf("after fseek(0)(%d) >>> ch = %c, read_cnt = %d, off = %d, %s, offset=%d, %d\n\n",
		__LINE__, ch, (int) fp->read_cnt, offset, strerror(errno), (int) fp->offset, (int) fp->sys_offset);

	offset = acl_vstream_fseek(fp, 10, SEEK_CUR);
	off2 = lseek(ACL_VSTREAM_FILE(fp), 0, SEEK_CUR);
	printf("after fseek(10)(%d) >>> off=%d, %d, lseek = %d, read_cnt = %d, offset=%d, %d\n\n",
		__LINE__, offset, (int) fp->sys_offset, off2, (int) fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);

	n = acl_vstream_gets(fp, buf, sizeof(buf) - 1);
	printf("after read(getline)(%d) >>> buf = [%s], n = %d, fp->read_cnt = %d, offset=%d, %d, lseek=%d\n",
		__LINE__, buf, n, (int) fp->read_cnt, (int) fp->offset, (int)fp->sys_offset,
		(int) lseek(ACL_VSTREAM_FILE(fp), 0, SEEK_CUR));
	offset = acl_vstream_fseek(fp, 0, SEEK_CUR);
	printf("after fseek(0)(%d) >>> buf = [%s], read_cnt = %d, off = %d, %d, offset=%d, %d\n\n",
		__LINE__, buf, (int) fp->read_cnt, offset, (int) lseek(ACL_VSTREAM_FILE(fp), 0, SEEK_CUR),
		(int) fp->offset, (int) fp->sys_offset);

	ch = acl_vstream_getc(fp);
	printf("after read(1)(%d) >>> ch = %c, read_cnt = %d, offset=%d, %d\n",
		__LINE__, ch, (int) fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);
	offset = acl_vstream_fseek(fp, 0, SEEK_CUR);
	printf("after fseek(0)(%d) >>> ch = %c, read_cnt = %d, off = %d, %d, offset=%d, %d\n\n",
		__LINE__, ch, (int) fp->read_cnt, offset, (int) lseek(ACL_VSTREAM_FILE(fp), 0, SEEK_CUR),
		(int) fp->offset, (int) fp->sys_offset);

	n = acl_vstream_readn(fp, buf, 2);
	buf[n] = 0;
	printf("after read(buf)(%d) >>> buf = [%s], fp->read_cnt = %d, offset=%d, %d\n",
		__LINE__, buf, (int) fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);
	offset = acl_vstream_fseek(fp, 10, SEEK_CUR);
	printf("after fseek(10)(%d) >>> read_cnt = %d, off = %d, offset=%d, %d\n\n",
		__LINE__, (int) fp->read_cnt, offset, (int) fp->offset, (int) fp->sys_offset);

	ch = acl_vstream_getc(fp);
	printf("after read(1)(%d) >>> ch = %c, read_cnt = %d, offset=%d, %d\n",
		__LINE__, ch, (int) fp->read_cnt, (int) fp->offset, (int) fp->sys_offset);
	offset = acl_vstream_fseek(fp, 0, SEEK_CUR);
	printf("after fseek(0)(%d) >>> ch = %c, read_cnt = %d, off = %d, %s, offset=%d, %d\n\n",
		__LINE__, ch, (int) fp->read_cnt, offset, strerror(errno), (int) fp->offset, (int) fp->sys_offset);

	acl_vstring_free(why);
	acl_vstream_close(fp);
}

int main(void)
{
	test_fseek(4);
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	test_fseek2(4);
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	test_fseek3(4);

	exit (0);
}
