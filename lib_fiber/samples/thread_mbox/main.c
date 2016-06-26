#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/lib_fiber.h"

static int __oper_count = 2;
static int __fibers_max = 2;
static int __fibers_cur = 2;

static void *thread_main(void *ctx)
{
	ACL_MBOX *mbox = (ACL_MBOX *) ctx;
	int   i;

	for (i = 0; i < __oper_count; i++) {
		char *ptr = acl_mystrdup("hello world!");
		if (acl_mbox_send(mbox, ptr) < 0) {
			printf("send error!\r\n");
			break;
		}
	}

	return NULL;
}

static void free_msg(void *msg)
{
	printf("---free one ---\r\n");
	acl_myfree(msg);
}

static void fiber_main(FIBER *fiber acl_unused, void *ctx)
{
	ACL_MBOX *mbox = (ACL_MBOX *) ctx;
	acl_pthread_attr_t attr;
	acl_pthread_t tid;
	int  i;

	acl_pthread_attr_init(&attr);
	acl_pthread_attr_setdetachstate(&attr, ACL_PTHREAD_CREATE_DETACHED);
	acl_pthread_create(&tid, &attr, thread_main, mbox);

	for (i = 0; i < __oper_count; i++) {
		char *ptr = (char *) acl_mbox_read(mbox, 0, NULL);
		if (ptr == NULL)
			break;
		if (i < 10)
			printf("read in: %s\r\n", ptr);
		acl_myfree(ptr);
	}

	printf(">>>nsend: %d / %d, nread: %d / %d\r\n",
		(int) acl_mbox_nsend(mbox), __oper_count,
		(int) acl_mbox_nread(mbox), __oper_count);

	acl_mbox_free(mbox, free_msg);

	if (--__fibers_cur == 0) {
		printf("---- All over now! ----\r\n");
		printf("Enter any key to exit ...");
		fflush(stdout);
		getchar();

		fiber_io_stop();
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -c nfibers\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, i;

	while ((ch = getopt(argc, argv, "hc:n:")) > 0) {
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
		default:
			break;
		}
	}

	__fibers_cur = __fibers_max;
	printf("fibers: %d\r\n", __fibers_max);

	for (i = 0; i < __fibers_max; i++) {
		ACL_MBOX *mbox = acl_mbox_create();
		fiber_create(fiber_main, mbox, 64000);
	}

	fiber_schedule();

	return 0;
}
