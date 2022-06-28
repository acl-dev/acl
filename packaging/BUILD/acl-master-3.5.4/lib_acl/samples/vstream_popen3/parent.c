#include "lib_acl.h"
#include <signal.h>

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

	ACL_VSTREAM_SET_RWTIMO(fp, 1);

	while (1) {
		ret = acl_vstream_gets_nonl(fp, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF) {
			printf("(parent): read over(%s), errno: %d, ACL_ETIMEDOUT: %d\n",
				acl_last_serror(), errno, ACL_ETIMEDOUT);
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
	if (errno == ACL_ETIMEDOUT)
#if 1
		kill(fp->pid, SIGKILL);
#else
		kill(fp->pid, SIGTERM);
#endif
	acl_vstream_pclose(fp);
	return (0);
}
