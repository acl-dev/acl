#include <getopt.h>
#include "lib_acl.h"

static char  __addr[128];
static int   __len = 1024;

static void thread_run(void *arg)
{
	int   i, n = *((int*) arg);
	ACL_VSTREAM *client = acl_vstream_connect(__addr,
			ACL_BLOCKING, 60, 60, 1024);
	char *buf, *line;
	int   len;

	printf("tid: %ld\r\n", (long) acl_pthread_self());

	if (client == NULL) {
		printf("connect %s error\r\n", __addr);
		return;
	}

	buf = acl_mymalloc(__len + 3);
	for (i = 0; i < __len; i++)
		buf[i] = 'x';
	buf[i] = '\r';
	buf[i + 1] = '\n';
	buf[i + 2] = 0;

	len = strlen(buf);
	line = acl_mymalloc(len + 1);

	for (i = 0; i < n; i++) {
		if (acl_vstream_writen(client, buf, strlen(buf)) == ACL_VSTREAM_EOF)
		{
			printf("write error\r\n");
			break;
		}
		if (acl_vstream_gets(client, line, len + 1) == ACL_VSTREAM_EOF)
		{
			printf("gets error\r\n");
			break;
		}
		if (n <= 100 || (i > 0 && i % 1000 == 0))
			printf("tid: %ld, i: %d, n: %d\r\n",
				(long) acl_pthread_self(), i, n);
	}
	acl_myfree(buf);
	acl_myfree(line);
	acl_vstream_close(client);
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -s addr[ip:port] -n nthreads -c count_per_thread -l line_length[1024]\r\n", procname);
}

int main(int argc, char **argv)
{
	int   ch, nthreads = 2, count = 100, i;
	acl_pthread_pool_t *thrpool;

	snprintf(__addr, sizeof(__addr), "127.0.0.1:5001");

	while ((ch = getopt(argc, argv, "hs:n:c:l:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(__addr, sizeof(__addr), "%s", optarg);
			break;
		case 'n':
			nthreads = atoi(optarg);
			if (nthreads <= 0)
				nthreads = 2;
			break;
		case 'c':
			count = atoi(optarg);
			if (count <= 0)
				count = 100;
			break;
		case 'l':
			__len = atoi(optarg);
			if (__len <= 0)
				__len = 1024;
			break;
		default:
			break;
		}
	}

	thrpool = acl_thread_pool_create(nthreads, 120);

	ACL_METER_TIME("----begin----");
	for (i = 0; i < nthreads; i++)
		acl_pthread_pool_add(thrpool, thread_run, &count);

	acl_pthread_pool_destroy(thrpool);
	ACL_METER_TIME("----end----");
	return 0;
}
