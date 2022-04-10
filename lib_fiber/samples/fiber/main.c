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

static __thread struct timeval __begin;
static __thread long long __count = 0;

#define	DUMMY_SIZE	512000

static void stack_dummy(ACL_FIBER *fiber acl_unused)
{
	char buf[DUMMY_SIZE];

	memset(buf, 0, sizeof(buf));
	printf("%s: called OK, dummy_size=%d\r\n", __FUNCTION__, DUMMY_SIZE);
}

static void fiber_main(ACL_FIBER *fiber, void *ctx acl_unused)
{
	int  i;
	size_t shared_stack_size = acl_fiber_get_shared_stack_size();

	printf("\r\nshared_stack_size=%zd\r\n\r\n", shared_stack_size);

	if (acl_fiber_use_share_stack(fiber) && shared_stack_size > DUMMY_SIZE) {
		stack_dummy(fiber);
	}

	errno = acl_fiber_errno(fiber);

	for (i = 0; i < __max_loop; i++) {
		if (__count < 10) {
			printf("fiber-%d, run, begin to yield\r\n",
				acl_fiber_id(fiber));
		}

		acl_fiber_yield();

		if (__count++ < 10) {
			printf("fiber-%d, wakeup errno: %d\r\n",
				acl_fiber_id(fiber), errno);
			printf("---------------------------------------\r\n");
		}
	}

	printf("%s: fiber-%d exiting, count=%lld ...\r\n",
		__FUNCTION__, acl_fiber_id(fiber), __count);
}

static void *thread_main(void *ctx)
{
	ACL_FIBER_ATTR *attr = (ACL_FIBER_ATTR *) ctx;
	int i;

	for (i = 0; i < __max_fiber; i++) {
		acl_fiber_create2(attr, fiber_main, NULL);
	}

	acl_fiber_schedule();

	printf("thread: %lu\r\n", (unsigned long) acl_pthread_self());
	return NULL;
}

static void calc_speed(void)
{
	long long count = __max_fiber * __max_loop;
	struct timeval end;
	double spent, speed;

	gettimeofday(&end, NULL);
	spent = stamp_sub(&end, &__begin);
	speed = (count * 1000) / (spent > 0 ? spent : 1);
	printf("\r\nfibers: %d, count: %lld, spent: %.2f, speed: %.2f\r\n",
		__max_fiber, count, spent, speed);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -n max_loop\r\n"
		" -c max_fiber\r\n"
		" -t max_threads\r\n"
		" -d stack_size\r\n"
		" -e [if display]\r\n"
		" -S [if use shared stack]\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	int   ch, i, nthreads = 1;
	acl_pthread_attr_t attr;
	acl_pthread_t *tids;
	ACL_FIBER_ATTR fiber_attr;
	size_t stack_size = 64000;

	acl_fiber_attr_init(&fiber_attr);

	while ((ch = getopt(argc, argv, "hn:c:t:ed:S")) > 0) {
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
			stack_size = (size_t) atoi(optarg);
			acl_fiber_attr_setstacksize(&fiber_attr, stack_size);
			break;
		case 'S':
			acl_fiber_attr_setsharestack(&fiber_attr, 1);
			break;
		case 'e':
			__display = 1;
			break;
		default:
			break;
		}
	}

	acl_fiber_set_shared_stack_size(8000000);

	acl_pthread_attr_init(&attr);
	tids = (acl_pthread_t *) acl_mycalloc(nthreads, sizeof(acl_pthread_t));

	gettimeofday(&__begin, NULL);

	for (i = 0; i < nthreads; i++) {
		acl_pthread_create(&tids[i], &attr, thread_main, &fiber_attr);
	}

	for (i = 0; i < nthreads; i++) {
		acl_pthread_join(tids[i], NULL);
	}

	calc_speed();
	printf("All fiber threads exited now ...\r\n");

	acl_myfree(tids);

#if defined(_WIN32) || defined(_WIN64)
	printf("enter any key to exit ..."); fflush(stdout);
	getchar();
#endif
	return 0;
}
