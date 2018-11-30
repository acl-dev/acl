#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32) && !defined(_WIN64)
#include <unistd.h>
#endif
#include "fiber/libfiber.h"
#include "stamp.h"

static int __max_loop = 1000;
static int __max_fiber = 1000;
static int __display   = 0;
static int __stack_size = 64000;

static __thread struct timeval __begin;
static __thread int __left_fiber = 1000;

static void stack_dummy(ACL_FIBER *fiber acl_unused)
{
	char buf[81920];

	memset(buf, 0, sizeof(buf));
}

static void fiber_main(ACL_FIBER *fiber, void *ctx acl_unused)
{
	int  i;

	if (0)
		stack_dummy(fiber);

	errno = acl_fiber_errno(fiber);
	for (i = 0; i < __max_loop; i++) {
		acl_fiber_yield();
		if (!__display)
			continue;

		if (i <= 2)
			printf("fiber-%d, errno: %d\r\n",
				acl_fiber_id(fiber), errno);
	}

	if (--__left_fiber == 0) {
		long long count = __max_fiber * __max_loop;
		struct timeval end;
		double spent;

		gettimeofday(&end, NULL);
		spent = stamp_sub(&end, &__begin);
		printf("fibers: %d, count: %lld, spent: %.2f, speed: %.2f\r\n",
			__max_fiber, count, spent,
			(count * 1000) / (spent > 0 ? spent : 1));
	}
}

static void *thread_main(void *ctx acl_unused)
{
	int i;

	gettimeofday(&__begin, NULL);
	__left_fiber = __max_fiber;

	for (i = 0; i < __max_fiber; i++)
		acl_fiber_create(fiber_main, NULL, __stack_size);

	acl_fiber_schedule();

	printf("thread: %lu\r\n", (unsigned long) acl_pthread_self());

	return NULL;
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -n max_loop\r\n"
		" -c max_fiber\r\n"
		" -t max_threads\r\n"
		" -d stack_size\r\n"
		" -e [if display]\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, i, nthreads = 1;
	acl_pthread_attr_t attr;
	acl_pthread_t *tids;

	while ((ch = getopt(argc, argv, "hn:c:t:d:e")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			__max_loop = atoi(optarg);
			break;
		case 'c':
			__max_fiber = atoi(optarg);
			break;
		case 't':
			nthreads = atoi(optarg);
			if (nthreads <= 0)
				nthreads = 1;
			break;
		case 'd':
			__stack_size = atoi(optarg);
			break;
		case 'e':
			__display = 1;
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

#if defined(_WIN32) || defined(_WIN64)
	printf("enter any key to exit ..."); fflush(stdout);
	getchar();
#endif
	return 0;
}
