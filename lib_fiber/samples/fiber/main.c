#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/lib_fiber.h"

static int __max_loop = 1000;
static int __max_fiber = 1000;

static __thread struct timeval __begin;
static __thread int __left_fiber = 1000;

static double stamp_sub(const struct timeval *from, const struct timeval *sub_by)
{
	struct timeval res;

	memcpy(&res, from, sizeof(struct timeval));

	res.tv_usec -= sub_by->tv_usec;
	if (res.tv_usec < 0) {
		--res.tv_sec;
		res.tv_usec += 1000000;
	}
	res.tv_sec -= sub_by->tv_sec;

	return (res.tv_sec * 1000.0 + res.tv_usec/1000.0);
}

static void fiber_main(FIBER *fiber, void *ctx acl_unused)
{
	int  i;

	errno = fiber_id(fiber);
	for (i = 0; i < __max_loop; i++) {
		fiber_yield();
		if (i <= 2)
			printf("fiber-%d, errno: %d\r\n", fiber_id(fiber), errno);
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

	printf("thread: %lu\r\n", (unsigned long) acl_pthread_self());
	for (i = 0; i < __max_fiber; i++)
		fiber_create(fiber_main, NULL, 32768);

	fiber_schedule();

	return NULL;
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -n max_loop -m max_fiber -t max_threads\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, i, nthreads = 1;
	acl_pthread_attr_t attr;
	acl_pthread_t *tids;

	while ((ch = getopt(argc, argv, "hn:m:t:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			__max_loop = atoi(optarg);
			break;
		case 'm':
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
