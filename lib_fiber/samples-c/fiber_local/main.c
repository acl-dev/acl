#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/libfiber.h"

static int __max_fiber = 10;
static __thread int __left_fiber = 10;
static __thread int __local_key;

static void free_fn(void *ctx)
{
	printf("thread-%ld, fiber-%d: free buf\r\n",
		(long) acl_pthread_self(), acl_fiber_self());
	acl_myfree(ctx);
}

static void fiber_main(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
	char *buf = (char *) acl_fiber_get_specific(__local_key);

	if (buf == NULL) {
		buf = acl_mystrdup("hello world!");
		acl_assert(acl_fiber_set_specific(&__local_key,  buf, free_fn) > 0);
		printf("fiber-%d: set local ok\r\n", acl_fiber_self());
	}

	buf = (char *) acl_fiber_get_specific(__local_key);
	if (buf == NULL) {
		printf("acl_fiber_get_specific NULL, __local_key: %d\r\n",
			__local_key);
		exit (1);
	}

	printf("thread-%ld, fiber-%d: __local_key: %d, buf: %s\r\n",
		(long) acl_pthread_self(), acl_fiber_self(), __local_key, buf);
	return;

	if (--__left_fiber == 0) {
		printf("---- acl_fiber_schedule_stop now ----\r\n");
		acl_fiber_schedule_stop();
	}
}

static void *thread_main(void *ctx acl_unused)
{
	int i;

	__left_fiber = __max_fiber;

	for (i = 0; i < __max_fiber; i++)
		acl_fiber_create(fiber_main, NULL, 64000);

	acl_fiber_schedule();

	printf("thread: %lu\r\n", (unsigned long) acl_pthread_self());

	return NULL;
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -c max_fiber\r\n"
		" -t max_threads\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, i, nthreads = 2;
	acl_pthread_attr_t attr;
	acl_pthread_t *tids;

	while ((ch = getopt(argc, argv, "hc:t:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			__max_fiber = atoi(optarg);
			break;
		case 't':
			nthreads = atoi(optarg);
			if (nthreads <= 0)
				nthreads = 1;
			break;
		default:
			break;
		}
	}

	acl_pthread_attr_init(&attr);
	tids = (acl_pthread_t *) acl_mycalloc(nthreads, sizeof(acl_pthread_t));

	for (i = 0; i < nthreads; i++)
		acl_pthread_create(&tids[i], &attr, thread_main, NULL);

	for (i = 0; i < nthreads; i++)
		acl_pthread_join(tids[i], NULL);

	acl_myfree(tids);

	return 0;
}
