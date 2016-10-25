#include "lib_acl.h"
#include <assert.h>

static void end(void)
{
#ifdef	ACL_MS_WINDOWS
	printf("input any key to exit\r\n");
	fflush(stdout);
	getchar();
#endif
	acl_lib_end();
}

static void test_file(void)
{
	const char *filename = "test2.txt";
	ACL_FILE *fp = acl_fopen(filename, "a+");
	char  buf[1024];
	int   i;

	if (fp == NULL) {
		printf("fopen %s error(%s)\n", filename, acl_last_serror());
		return;
	}

	for (i = 0 ; i < 100; i++) {
		if (acl_fputs("hello world!", fp) == EOF) {
			printf("fputs to %s error(%s)\n", filename, acl_last_serror());
			break;
		}
	}

	printf("write to %s ok\n", filename);
	acl_fclose(fp);

	fp = acl_fopen(filename, "r");
	if (fp == NULL) {
		printf("fopen %s error(%s)\n", filename, acl_last_serror());
		return;
	}

	i = 0;
	while (!acl_feof(fp)) {
		if (acl_fgets(buf, sizeof(buf), fp) == NULL) {
			printf("fgets return null, i=%d\n", i);
		} else {
			i++;
			printf(">>gets line(%d): %s", i, buf);
		}
	}
	acl_fclose(fp);
}

static int test_file3(void)
{
	char  buf[1024];
	int   ret;

	ACL_VSTREAM *fp = acl_vstream_fopen("test3.txt", O_RDONLY, 0700, 8192);
	if (fp == NULL) {
		printf("fopen error %s\n", acl_last_serror());
		return (-1);
	}

	while (1) {
		ret = acl_vstream_gets_nonl(fp, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF)
			break;
		printf(">>>gets(%s), ret: %d\n", buf, ret);
	}

	acl_vstream_fclose(fp);
	return (0);
}

static int test_vstream(void)
{
	const char *filename = "test.txt";
	ACL_VSTREAM *fp = acl_vstream_fopen(filename, O_RDWR | O_CREAT, 0644, 1024);
	struct acl_stat sbuf;
	char  buf[256];
	struct tm *local_time;

	acl_lib_init();

	if (fp == NULL) {
		printf("open file(%s) error(%s)\r\n", filename, acl_last_serror());
		end();
		return (-1);
	}

	if (acl_vstream_fstat(fp, &sbuf) < 0) {
		acl_vstream_close(fp);
		printf("fstat file(%s) error(%s)\r\n", filename, acl_last_serror());
		end();
		return (-1);
	}

	printf("file(%s) stat:\r\n", filename);

	printf("size=%d\r\n", (int) sbuf.st_size);

	local_time = localtime(&sbuf.st_mtime);
	if (local_time) {
		strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", local_time);
		printf("修改时间，mtime=%s\r\n", buf);
	} else {
		printf("mtime: error(%s)\r\n", acl_last_serror());
	}

	local_time = localtime(&sbuf.st_ctime);
	if (local_time) {
		strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", local_time);
		printf("创建时间，ctime=%s\r\n", buf);
	} else {
		printf("ctime: error(%s)\r\n", acl_last_serror());
	}

	local_time = localtime(&sbuf.st_atime);
	if (local_time) {
		strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", local_time);
		printf("访问时间，atime=%s\r\n", buf);
	} else {
		printf("atime: error(%s)\r\n", acl_last_serror());
	}

	int  ch, ch1;

	ch = ACL_VSTREAM_GETC(fp);
	ch = 'a';

	for (int i = 0; i < 100; i++) {
		ch1 = ACL_VSTREAM_PUTC(ch, fp);
		assert(ch1 == ch);
		printf("ok write char: %c\n", ch1);
	}
	ACL_VSTREAM_PUTC('\n', fp);
	acl_vstream_fclose(fp);
	end();	

	FILE *fp1 = fopen("test.txt", "w+");
	assert(fp1);
	int ret = fputs("hello", fp1);
	printf("ret: %d\n", ret);
	fclose(fp1);
	return (0);
}

int main(int argc acl_unused, char *argv[] acl_unused)
{
	(void) test_vstream();
	test_file();
	test_file3();
	return (0);
}

