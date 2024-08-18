#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/libfiber.h"
#include "stamp.h"

static int __timer_sleep = 1000;
static int __max_fiber   = 1000;

static __thread struct timeval __begin;
static __thread int __left_fiber = 1000;

static void timer_main(ACL_FIBER *fiber, void *ctx acl_unused)
{
	struct timeval now;
	double spent;

	gettimeofday(&now, NULL);
	spent = stamp_sub(&now, &__begin);

	printf("thread-%lu, timer-%d wakeup, spend: %.2f ms\r\n",
		(unsigned long) acl_pthread_self(), acl_fiber_id(fiber), spent);

	if (--__left_fiber == 0) {
		printf("All are over!\r\n");
		//acl_fiber_schedule_stop();
	}
}

static void *thread_main(void *ctx acl_unused)
{
	int i;

	gettimeofday(&__begin, NULL);
	__left_fiber = __max_fiber;

	printf("thread: %lu\r\n", (unsigned long) acl_pthread_self());
	for (i = 0; i < __max_fiber; i++) {
		printf("--- create one timer ---\r\n");
		acl_fiber_create_timer(__timer_sleep, 320000, timer_main, NULL);
	}

	acl_fiber_schedule();

	return NULL;
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -w timer_sleep\r\n"
		" -m max_fiber\r\n"
		" -t max_threads\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, i, nthreads = 1;
	acl_pthread_attr_t attr;
	acl_pthread_t *tids;

	while ((ch = getopt(argc, argv, "hm:t:w:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'm':
			__max_fiber = atoi(optarg);
			break;
		case 't':
			nthreads = atoi(optarg);
			if (nthreads <= 0)
				nthreads = 1;
			break;
		case 'w':
			__timer_sleep = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl_fiber_msg_stdout_enable(1);
	acl_pthread_attr_init(&attr);
	tids = (acl_pthread_t *) acl_mycalloc(nthreads, sizeof(acl_pthread_t));

	for (i = 0; i < nthreads; i++)
		acl_pthread_create(&tids[i], &attr, thread_main, NULL);

	for (i = 0; i < nthreads; i++)
		acl_pthread_join(tids[i], NULL);

	acl_myfree(tids);

	return 0;
}
