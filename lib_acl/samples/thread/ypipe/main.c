#include "lib_acl.h"
#include <time.h>

static int  __max = 100000000;
static int  __base = 10;
static char __dummy[256];

static void *thread_producer(void *arg)
{
	ACL_YPIPE *ypipe = (ACL_YPIPE*) arg;
	int   i, j, n = __max / __base;

	for (i = 0; i < n; i++) {
		for (j = 0; j < __base; j++)
			acl_ypipe_write(ypipe, __dummy);
		acl_ypipe_flush(ypipe);
	}

	return NULL;
}

static void *thread_consumer(void *arg)
{
	ACL_YPIPE *ypipe = (ACL_YPIPE*) arg;
	int   i, n = 0;

	for (i = 0; i < __max; i++) {
		char *ptr = (char*) acl_ypipe_read(ypipe);
		if (ptr != NULL)
			n++;
		else {
			i--;
			//printf("ptr NULL, i: %d\r\n", i);
		}
	}

	printf("i: %d, n: %d\r\n", i, n);
	return NULL;
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -n max -b base\r\n", procname);
}

int main(int argc, char *argv[])
{
	acl_pthread_attr_t attr;
	acl_pthread_t t1, t2;
	ACL_YPIPE *ypipe = acl_ypipe_new();
	int   ch;

	while ((ch = getopt(argc, argv, "hn:b:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			__max = atoi(optarg);
			break;
		case 'b':
			__base = atoi(optarg);
			break;
		default:
			break;
		}
	}

	memset(__dummy, 'x', sizeof(__dummy));
	__dummy[sizeof(__dummy) - 1] = 0;

	acl_pthread_attr_init(&attr);
	acl_pthread_create(&t2, &attr, thread_consumer, ypipe);
	acl_pthread_create(&t1, &attr, thread_producer, ypipe);
	acl_pthread_join(t2, NULL);
	acl_pthread_join(t1, NULL);

	acl_ypipe_free(ypipe, NULL);
	printf("over\r\n");

	return 0;
}
