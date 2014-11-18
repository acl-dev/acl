#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>

#include "test_stdlib.h"

int test_file_vstream(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	ACL_VSTREAM *fp = NULL;
	const char *filename;
	char  buf[1024];
	int   i, n = 4, ret;

#undef	RETURN
#define	RETURN(_x_) do {  \
	if (fp)  \
		acl_vstream_fclose(fp);  \
	return (_x_);  \
} while (0)

	AUT_SET_STR(test_line, "filename", filename);
	fp = acl_vstream_fopen(filename, O_CREAT | O_RDWR | O_TRUNC, 0600, 1024);
	if (fp == NULL) {
		printf("fopen file(%s) error(%s)\r\n",
			filename, acl_last_serror());
		return (-1);
	}

	memset(buf, 'X', sizeof(buf));
	for (i = 0; i < n; i++) {
		if (acl_vstream_buffed_writen(fp, buf, sizeof(buf)) == ACL_VSTREAM_EOF) {
			printf("buffed write to file(%s) error(%s)\r\n",
				filename, acl_last_serror());
			RETURN (-1);
		}
	}

	ret = (int) acl_vstream_fsize(fp);
	if (ret != (int) sizeof(buf) * n) {
		printf("%s's fsize return error(%s)\r\n", filename, acl_last_serror());
		RETURN (-1);
	}
	printf("%s's acl_vstream_fsize: %d\r\n", filename, ret);

	i = (int) acl_file_size(filename);
	printf("%s: fsize=%d before fflush\r\n", filename, i);

#if 1
	if (acl_vstream_fflush(fp) < 0) {
		printf("fflush to file(%s) error(%s)\r\n",
			filename, acl_last_serror());
		RETURN (-1);
	}
	i = (int) acl_file_size(filename);
	printf("%s: fsize=%d after fflush\r\n", filename, i);
#endif

	if (acl_vstream_fsync(fp) == ACL_VSTREAM_EOF) {
		printf("fsync to file(%s) error(%s)\r\n",
			filename, acl_last_serror());
		RETURN (-1);
	}
	i = (int) acl_file_size(filename);
	printf("%s: fsize=%d after fsync\r\n", filename, i);

	RETURN (0);
}

