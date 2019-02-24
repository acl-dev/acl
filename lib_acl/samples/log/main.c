#include "lib_acl.h"
#include <getopt.h>

static int __num = 10;

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -t logger_files -n num -N nthreads\n", procname);
	printf("examples: %s -t TCP:127.0.0.1:12345|UDP:127.0.0.1:12345|./test.log -n 100 -N 2\n", procname);
}

static void thread_main(void *arg acl_unused)
{
	int   i = 0;

	for (i = 0; i < __num; i++)
		acl_msg_info("%s(%d), log %d", __FILE__, __LINE__, i);
}

int main(int argc, char *argv[])
{
	int  i, ch, nthread = 1;
	char  buf[1024];
	acl_pthread_pool_t *pool;

	snprintf(buf, sizeof(buf), "test.log");
	while ((ch = getopt(argc, argv, "ht:n:N:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 't':
			snprintf(buf, sizeof(buf), "%s", optarg);
			break;
		case 'n':
			__num = atoi(optarg);
			break;
		case 'N':
			nthread = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl_msg_open(buf, "log_test");

	pool = acl_thread_pool_create(nthread, 100);
	for (i = 0; i < nthread; i++)
		acl_pthread_pool_add(pool, thread_main, NULL);
	acl_pthread_pool_destroy(pool);

	acl_msg_close();
	return (0);
}
