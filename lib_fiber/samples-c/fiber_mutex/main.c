#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "fiber/libfiber.h"
#include "../stamp.h"

static int __fibers_count = 10;
static int __event_type = FIBER_EVENT_KERNEL;

static void fiber_main(ACL_FIBER *fiber, void *ctx)
{
	ACL_FIBER_MUTEX *l = (ACL_FIBER_MUTEX *) ctx;

	printf("fiber-%d begin to lock\r\n", acl_fiber_id(fiber));
	acl_fiber_mutex_lock(l);
	printf("fiber-%d lock ok\r\n", acl_fiber_id(fiber));

	printf("fiber-%d begin sleep\r\n", acl_fiber_id(fiber));
	acl_fiber_sleep(1);
	printf("fiber-%d wakeup\r\n", acl_fiber_id(fiber));

	acl_fiber_mutex_unlock(l);
	printf("fiber-%d unlock ok\r\n", acl_fiber_id(fiber));

	if (--__fibers_count == 0) {
		printf("--- All fibers Over ----\r\n");
		//acl_fiber_schedule_stop();
	}
}

static void test1(void)
{
	int i;
	ACL_FIBER_MUTEX *l = acl_fiber_mutex_create(0);

	for (i = 0; i < __fibers_count; i++) {
		acl_fiber_create(fiber_main, l, 320000);
	}

	acl_fiber_schedule_with(__event_type);
	acl_fiber_mutex_free(l);
}

//////////////////////////////////////////////////////////////////////////////

static int __use_yield = 0;
static int __use_event = 0;
static ACL_FIBER_MUTEX **__locks;
static ACL_FIBER_EVENT **__events;
static int __nlocks = 10, __nloop = 100;
static __thread int __nfibers = 10;
static ACL_ATOMIC *__atomic;
static long long __atomic_value;
static __thread int __count = 0;

static void fiber_main2(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
	int i, ret;
	ACL_FIBER_MUTEX *l;

	//printf("thread-%lu, fiber-%d start!\r\n", (long) pthread_self(), acl_fiber_self());

	for (i = 0; i < __nloop; i++) {
		l = __locks[i % __nlocks];
		ret = acl_fiber_mutex_lock(l);
		if (ret) {
			printf("%s: lock error=%s\r\n", __FUNCTION__, strerror(ret));
			assert(ret == 0);
		}

		//acl_fiber_delay(100);

		ret = acl_fiber_mutex_unlock(l);

		//printf(">>fiber-%d unlock\n", acl_fiber_self());
		if (ret) {
			printf("%s: unlock error=%s\r\n", __FUNCTION__, strerror(ret));
			assert(ret == 0);
		}

		acl_atomic_int64_add_fetch(__atomic, 1);
		if (__use_yield) {
			acl_fiber_yield();
		}
		__count++;

		//acl_fiber_delay(10);
	}

	if (--__nfibers == 0) {
		printf("%s: thread-%lu, all fibers over, count=%d!\r\n",
			__FUNCTION__, (unsigned long) pthread_self(), __count);
		//acl_fiber_schedule_stop();
	}
}

static void fiber_main3(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
	int i;
	ACL_FIBER_EVENT *l;

	for (i = 0; i < __nloop; i++) {
		l = __events[i % __nlocks];
		acl_fiber_event_wait(l);
		acl_fiber_event_notify(l);
		acl_atomic_int64_add_fetch(__atomic, 1);
		if (__use_yield) {
			acl_fiber_yield();
		}
		__count++;
	}

	if (--__nfibers == 0) {
		printf("%s: thread-%lu, all fibers over, count=%d\r\n",
			__FUNCTION__, (unsigned long) pthread_self(), __count);
		//acl_fiber_schedule_stop();
	}
}

static void *thread_main(void *arg acl_unused)
{
	int i;

	__nfibers = __fibers_count;

	for (i = 0; i < __fibers_count; i++) {
		if (__use_event) {
			acl_fiber_create(fiber_main3, NULL, 320000);
		} else {
			acl_fiber_create(fiber_main2, NULL, 320000);
		}
	}

	acl_fiber_schedule_with(__event_type);
	return NULL;
}

static void *thread_alone_main(void *ctx acl_unused)
{
	int i;
	ACL_FIBER_MUTEX *l;

	for (i = 0; i < __nloop; i++) {
		l = __locks[i % __nlocks];
		acl_fiber_mutex_lock(l);
		acl_fiber_mutex_unlock(l);
		acl_atomic_int64_add_fetch(__atomic, 1);
		__count++;
	}

	printf("thread-%lu over, count=%d\r\n",
		(unsigned long) pthread_self(), __count);
	return NULL;
}

static void test2(int nthreads, int nthreads2, unsigned flags)
{
	int i;
	pthread_t threads[nthreads], threads2[nthreads2];
	struct timeval begin, end;
	double cost, speed;
	long long n;

	__locks = (ACL_FIBER_MUTEX**) malloc(__nlocks * sizeof(ACL_FIBER_MUTEX*));
	__events = (ACL_FIBER_EVENT**) malloc(__nlocks * sizeof(ACL_FIBER_EVENT*));
	for (i = 0; i < __nlocks; i++) {
		__locks[i] = acl_fiber_mutex_create(flags);
		__events[i] = acl_fiber_event_create(FIBER_FLAG_USE_MUTEX);
	}

	__atomic = acl_atomic_new();
	acl_atomic_set(__atomic, &__atomic_value);
	acl_atomic_int64_set(__atomic, 0);

	gettimeofday(&begin, NULL);

	for (i = 0; i < nthreads; i++) {
		pthread_create(&threads[i], NULL, thread_main, NULL);
		printf("--create fibers thread-%lu\r\n", (long) threads[i]);
	}

	for (i = 0; i < nthreads2; i++) {
		pthread_create(&threads2[i], NULL, thread_alone_main, NULL);
		printf("--create alone thread-%lu\r\n", (long) threads2[i]);
	}

	for (i = 0; i < nthreads; i++) {
		pthread_join(threads[i], NULL);
	}

	for (i = 0; i < nthreads2; i++) {
		pthread_join(threads2[i], NULL);
	}

	gettimeofday(&end, NULL);

	n = acl_atomic_int64_fetch_add(__atomic, 0);
	cost = stamp_sub(&end, &begin);
	speed = (n * 1000) / (cost > 0 ? cost : 0.1);

	printf("---ALL OVER, count=%lld, cost=%.2f ms, speed=%.2f---\n",
		n, cost, speed);

	for (i = 0; i < __nlocks; i++) {
		acl_fiber_mutex_free(__locks[i]);
		acl_fiber_event_free(__events[i]);
	}

	free(__locks);
	free(__events);
	acl_atomic_free(__atomic);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -e schedule_event_type[kernel|poll|select|io_uring]\r\n"
		" -a action[test1|test, default: test2]\r\n"
		" -t threads_count\r\n"
		" -c fibers_count\r\n"
		" -p threads_alone_count\r\n"
		" -n loop_count\r\n"
		" -l locks_count\r\n"
		" -E [if use fiber_event]\r\n"
		" -Y [if yield after unlock]\r\n"
		" -T [if use first try lock for mutex]\r\n"
		" -S [if using fiber switching first when they're in the same thread]\r\n"
		, procname);
}

static void test_lock(void)
{
	int i;
	pthread_mutex_t lock;

	pthread_mutex_init(&lock, NULL);

	for (i = 0; i < 5; i++) {
		int ret = pthread_mutex_trylock(&lock);
		if (ret != 0) {
			printf("lock error %s, i=%d\n", strerror(ret), i);
			exit(1);
		}
		printf("lock ok, i=%d\n", i);
	}

	for (i = 0; i < 5; i++) {
		int ret = pthread_mutex_lock(&lock);
		if (ret != 0) {
			printf("unlock error, i=%d\n", i);
			exit(1);
		}
		printf("unlock ok, i=%d\n", i);
	}

}

int main(int argc, char *argv[])
{
	int  ch, nthreads = 2, nthreads2 = 0;
	unsigned flags = 0;
	char action[64];

	if (0) {
		test_lock();
	}

	snprintf(action, sizeof(action), "test2");

	while ((ch = getopt(argc, argv, "he:a:t:c:p:n:l:EYTS")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'e':
			if (strcasecmp(optarg, "poll") == 0) {
				__event_type = FIBER_EVENT_POLL;
			} else if (strcasecmp(optarg, "select") == 0) {
				__event_type = FIBER_EVENT_SELECT;
			} else if (strcasecmp(optarg, "io_uring") == 0) {
				__event_type = FIBER_EVENT_IO_URING;
			} else if (strcasecmp(optarg, "kernel") == 0) {
				__event_type = FIBER_EVENT_KERNEL;
			}
			break;
		case 'a':
			snprintf(action, sizeof(action), "%s", optarg);
			break;
		case 't':
			nthreads = atoi(optarg);
			break;
		case 'c':
			__fibers_count = atoi(optarg);
			break;
		case 'p':
			nthreads2 = atoi(optarg);
			break;
		case 'n':
			__nloop = atoi(optarg);
			break;
		case 'l':
			__nlocks = atoi(optarg);
			break;
		case 'E':
			__use_event = 1;
			break;
		case 'Y':
			__use_yield = 1;
			break;
		case 'T':
			flags |= FIBER_MUTEX_F_LOCK_TRY;
			break;
		case 'S':
			flags |= FIBER_MUTEX_F_SWITCH_FIRST;
			break;
		default:
			break;
		}
	}

	acl_fiber_msg_stdout_enable(1);

	if (strcasecmp(action, "test1") == 0) {
		test1();
	} else if (strcasecmp(action, "test2") == 0) {
		test2(nthreads, nthreads2, flags);
	} else {
		printf("unknown action: %s\r\n", action);
		usage(argv[0]);
	}
	return 0;
}
