#include "lib_acl.h"
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "mem_test.h"

typedef struct {
	char *p;
	int   n;
} A;

static A *create_a(size_t n)
{
	A *a = acl_mymalloc(sizeof(A));

	a->n = n;
	a->p = acl_mymalloc(a->n);

	return (a);
}

static void reset_a(A *a, size_t n)
{
	a->p = acl_mymalloc(n);
	a->n = n;
	a->p = acl_mymalloc(n);
	a->p = acl_mymalloc(n);
	a->p = acl_mymalloc(n);
}

static void thread_run(void *arg)
{
	A *a = (A*) arg;

	assert(a->n > 0);

#if 0
	acl_myfree(a->p);
	acl_myfree(a);
#endif
}

static void thread_test(void)
{
	acl_pthread_pool_t *thr_pool;
	int   i, n = 10000000;
	A *a;
	time_t last, begin;

	begin = time(NULL);
	last = begin;
	thr_pool = acl_thread_pool_create(20, 120);
	for (i = 0; i < n; i++) {
		if (i % 10000 == 0) {
			time_t t = time(NULL);
			printf("i: %d, time: %ld\n", i, t - last);
			last = t;
		}
		a = create_a(1024);
		acl_pthread_pool_add(thr_pool, thread_run, a);
	}

	acl_pthread_pool_destroy(thr_pool);
	printf("total: %d, time: %ld\n", i, time(NULL) - begin);
}

void mem_test()
{
	A *a;
	int i = 0, n = 1000, k = 1;
	
	for (i = 0; i < n; i++) {
		a = create_a(1024);
		reset_a(a, 2048);
		reset_a(a, 2048);
	}

	printf("begin thread_test\n");
	thread_test();

	printf("ok, sleep %d seconds\n", k);
	sleep(k);
	printf("ok, wakeup\n");
}
