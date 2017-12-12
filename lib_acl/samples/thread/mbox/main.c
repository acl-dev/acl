#include "lib_acl.h"
#include <time.h>

static long long int __max = 100000000;
static char __dummy[256];
static int  __use_dummy = 0;

static void *thread_producer(void *arg)
{
	ACL_MBOX *mbox = (ACL_MBOX *) arg;
	long long int i, n = __max;
	char *ptr;

	for (i = 0; i < n; i++) {
		if (__use_dummy)
			ptr = __dummy;
		else
			ptr = acl_mystrdup("hello world!");

		if (acl_mbox_send(mbox, ptr) < 0) {
			printf(">>send error\r\n");
			break;
		}
	}

	return NULL;
}

static void *thread_consumer(void *arg)
{
	ACL_MBOX *mbox = (ACL_MBOX *) arg;
	long long int i, n = 0;

	for (i = 0; i < __max; i++) {
		char *ptr = (char*) acl_mbox_read(mbox, 0, NULL);
		if (ptr != NULL) {
			n++;
			if (i < 10)
				printf(">>read: %s\r\n", ptr);
			if (ptr != __dummy)
				acl_myfree(ptr);
		} else {
			printf("ptr NULL, i: %lld\r\n", i);
			break;
		}
	}

	printf("hit ratio: %.2f %%, i: %lld, n: %lld, io send: %d, io read: %d\r\n",
		(double) (n - acl_mbox_nsend(mbox)) * 100 / (n > 0 ? n : 1),
		i, n, (int) acl_mbox_nsend(mbox), (int) acl_mbox_nread(mbox));
	return NULL;
}

static void free_msg(void *ctx)
{
	char *ptr = (char *) ctx;

	printf("---free one: %s ---\r\n", ptr);
	acl_myfree(ptr);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -n max -s [use static buffer]\r\n", procname);
}

int main(int argc, char *argv[])
{
	acl_pthread_attr_t attr;
	acl_pthread_t t1, t2;
	ACL_MBOX *mbox = acl_mbox_create();
	int   ch;

	while ((ch = getopt(argc, argv, "hn:s")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			__max = atoi(optarg);
			break;
		case 's':
			__use_dummy = 1;
			break;
		default:
			break;
		}
	}

	acl_msg_stdout_enable(1);
	memset(__dummy, 'x', sizeof(__dummy));
	__dummy[sizeof(__dummy) - 1] = 0;

	acl_pthread_attr_init(&attr);
	acl_pthread_create(&t2, &attr, thread_consumer, mbox);
	acl_pthread_create(&t1, &attr, thread_producer, mbox);
	acl_pthread_join(t2, NULL);
	acl_pthread_join(t1, NULL);

	acl_mbox_free(mbox, free_msg);

	printf("over\r\n");

	return 0;
}
