#include "lib_acl.h"

static void usage(const char *procname)
{
	printf("usage: %s child_progname\n", procname);
}

int main(int argc, char *argv[])
{
	ACL_VSTREAM *fp;
	char *command, buf[1024];
	int   ret, i = 0;

	if (argc != 2) {
		usage(argv[0]);
		return (0);
	}

	command = argv[1];

	fp = acl_vstream_popen(O_RDWR,
			ACL_VSTREAM_POPEN_COMMAND, command,
			ACL_VSTREAM_POPEN_END);

	while (1) {
		ret = acl_vstream_gets_nonl(fp, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF) {
			printf("(parent): read over(%s)\n", acl_last_serror());
			break;
		}
		printf("(parent): read from child(%s)\n", buf);
		ret = acl_vstream_fprintf(fp, "from parent ok\n");
		if (ret == ACL_VSTREAM_EOF) {
			printf("(parent): write error(%s)\n", acl_last_serror());
			break;
		}
		if (i++ > 10)
			break;
	}

	printf("(parent): over now\n");
	acl_vstream_pclose(fp);
	return (0);
}
