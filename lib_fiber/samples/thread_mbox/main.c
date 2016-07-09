#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/lib_fiber.h"
#include "stamp.h"

static long long int __oper_count = 2;
static int __fibers_max = 2;
static int __fibers_cur = 2;
static char __dummy[256];
static int  __use_dummy = 0;

static void *thread_main(void *ctx)
{
	ACL_MBOX *mbox = (ACL_MBOX *) ctx;
	char *ptr;
	int   i;

	snprintf(__dummy, sizeof(__dummy), "hello world");

	for (i = 0; i < __oper_count; i++) {
		if (__use_dummy)
			ptr = __dummy;
		else
			ptr = acl_mystrdup("hello world!");

		if (acl_mbox_send(mbox, ptr) < 0) {
			printf("send error!\r\n");
			break;
		}
	}

	printf("----- send over: %lld -----\r\n", __oper_count);

	return NULL;
}

static void free_msg(void *msg)
{
	printf("---fiber-%d: free one ---\r\n", acl_fiber_self());
	acl_myfree(msg);
}

static void fiber_main(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_MBOX *mbox = (ACL_MBOX *) ctx;
	acl_pthread_attr_t attr;
	acl_pthread_t tid;
	int  i;
	struct timeval begin, end;
	double spent;

	acl_pthread_attr_init(&attr);
	acl_pthread_attr_setdetachstate(&attr, ACL_PTHREAD_CREATE_DETACHED);
	acl_pthread_create(&tid, &attr, thread_main, mbox);

	gettimeofday(&begin, NULL);

	for (i = 0; i < __oper_count; i++) {
		char *ptr = (char *) acl_mbox_read(mbox, 0, NULL);
		if (ptr == NULL)
			break;
		if (i < 10)
			printf("--- read in: %s ---\r\n", ptr);
		if (ptr != __dummy)
			acl_myfree(ptr);
	}

	gettimeofday(&end, NULL);
	spent = stamp_sub(&end, &begin);

	printf(">>>hit ratio: %.2f %%, nsend: %d / %lld, nread: %d / %lld\r\n",
		(double) (__oper_count - acl_mbox_nsend(mbox)) * 100 / __oper_count,
		(int) acl_mbox_nsend(mbox), __oper_count,
		(int) acl_mbox_nread(mbox), __oper_count);
	printf("total: %lld, spend: %.2f, speed: %.2f\r\n", __oper_count,
		spent, (__oper_count * 1000) / (spent > 0 ? spent : 1));

	acl_mbox_free(mbox, free_msg);

	if (--__fibers_cur == 0) {
		printf("---- All over now! ----\r\n");
		printf("Enter any key to exit ...");
		fflush(stdout);
		getchar();

		acl_fiber_stop();
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -c nfibers -n count -s [use static buffer]\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, i;

	while ((ch = getopt(argc, argv, "hc:n:s")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			__fibers_max = atoi(optarg);
			break;
		case 'n':
			__oper_count = atoi(optarg);
			break;
		case 's':
			__use_dummy = 1;
			break;
		default:
			break;
		}
	}

	__fibers_cur = __fibers_max;
	printf("fibers: %d\r\n", __fibers_max);

	for (i = 0; i < __fibers_max; i++) {
		ACL_MBOX *mbox = acl_mbox_create();
		acl_fiber_create(fiber_main, mbox, 64000);
	}

	acl_fiber_schedule();

	return 0;
}
