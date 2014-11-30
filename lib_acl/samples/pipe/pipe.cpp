// pipe.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "lib_acl.h"

static void test(void)
{
	ACL_FILE_HANDLE fds[2];
	char  buf[1024];
	int   ret;

	if (acl_pipe(fds) < 0) {
		printf("acl_pipe error(%s)\n", acl_last_serror());
		return;
	}

	sprintf(buf, "hello client");
	ret = acl_file_write(fds[0], buf, strlen(buf), 0, 0, 0);
	if (ret == ACL_VSTREAM_EOF) {
		printf("write to client error(%s)\n", acl_last_serror());
		acl_pipe_close(fds);
		return;
	}
	printf(">>>server: write to client ok\n");

	ret = acl_file_read(fds[1], buf, sizeof(buf), 0, 0, 0);
	if (ret == ACL_VSTREAM_EOF) {
		printf(">>>client: read from server error(%s)\n", acl_last_serror());
		acl_pipe_close(fds);
		return;
	}
	buf[ret] = 0;
	printf(">>>client: read from server ok(%s)\n", buf);
	
	sprintf(buf, "hello server");
	ret = acl_file_write(fds[1], buf, strlen(buf), 0, 0, 0);
	if (ret == ACL_VSTREAM_EOF) {
		printf("write to server error(%s)\n", acl_last_serror());
		acl_pipe_close(fds);
		return;
	}
	printf(">>>client: write to server ok\n");

	ret = acl_file_read(fds[0], buf, sizeof(buf), 0, 0, 0);
	if (ret == ACL_VSTREAM_EOF) {
		printf(">>>server: read from client error(%s)\n", acl_last_serror());
		acl_pipe_close(fds);
		return;
	}
	printf(">>>server: read from client ok(%s)\n", buf);

	acl_pipe_close(fds);
}

int _tmain(int argc, _TCHAR* argv[])
{
	acl_init();
	test();
	getchar();
	acl_end();
	return 0;
}

