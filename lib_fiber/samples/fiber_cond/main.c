#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "fiber/libfiber.h"
#include "../stamp.h"

static int __event_type   = FIBER_EVENT_KERNEL;
static int __fibers_count = 1;
static int __total_count  = 10;
static int __wait_timeout = -1;

static ACL_FIBER_MUTEX *__mutex;
static ACL_FIBER_COND  *__cond;
static ACL_ATOMIC      *__atomic;
static long long        __atomic_value;
static ACL_ATOMIC      *__atomic_signal;
static long long        __atomic_signal_value;

static __thread int __nfibers   = 1;
static __thread int __count     = 0;
static int __all_consumers_exit = 0;

static void fiber_producer(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
	int n = 0;
	while (!__all_consumers_exit) {
		acl_fiber_cond_signal(__cond);

		acl_atomic_int64_add_fetch(__atomic_signal, 1);

		if (++n % 2000000 == 0) {
			//printf("--signal = %d\n", n);
			//acl_fiber_delay(1000);
		}
		//acl_fiber_delay(1000);
	}

	if (--__nfibers == 0) {
		printf("thread-%lu, all producers over, total signal=%lld!\r\n",
			pthread_self(), acl_atomic_int64_add_fetch(__atomic_signal, 0));
		//acl_fiber_schedule_stop();
	}
}

static void *thread_producer(void *arg acl_unused)
{
	int i;

	__nfibers = __fibers_count;

	for (i = 0; i < __fibers_count; i++) {
		acl_fiber_create(fiber_producer, NULL, 320000);
	}

	acl_fiber_schedule_with(__event_type);
	return NULL;
}

static void do_consume(void)
{
	int ret, base = __total_count / 10, res;
	long long n;

	if (base == 0) {
		base = 1;
	}

	while (1) {
		res = acl_fiber_mutex_lock(__mutex);
		if (res != 0) {
			printf("%s(%d): lock error=%s\r\n",
				__FUNCTION__, __LINE__, strerror(res));
		}

		ret = acl_fiber_cond_timedwait(__cond, __mutex, __wait_timeout);

		res = acl_fiber_mutex_unlock(__mutex);
		if (res != 0) {
			printf("%s(%d): unlock error=%s\r\n",
				__FUNCTION__, __LINE__, strerror(res));
		}

		if (ret != 0) {
			printf("thread-%lu, fiber-%d: timedwait error=%s\r\n",
				pthread_self(), acl_fiber_self(), strerror(ret));
			continue;
		}

		//printf("thread-%lu got one\n", pthread_self());

		if (++__count % base == 0) {
			printf("---consumer thread-%lu, fiber=%d: count=%d\r\n",
				pthread_self(), acl_fiber_self(), ++__count);
		}

		n = acl_atomic_int64_add_fetch(__atomic, 1);
		if (n >= __total_count) {
			printf("---thread-%lu, fiber=%d, consumer all ok, "
				"n=%lld, total=%d---\r\n", pthread_self(),
				acl_fiber_self(), n, __total_count);
			break;
		}
	}

	acl_fiber_mutex_unlock(__mutex);
}

static void fiber_consumer(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
	do_consume();

	if (--__nfibers == 0) {
		//acl_fiber_schedule_stop();
	}
}

static void *thread_consumer(void *arg acl_unused)
{
	int i;

	__nfibers = __fibers_count;

	for (i = 0; i < __fibers_count; i++) {
		acl_fiber_create(fiber_consumer, NULL, 320000);
	}

	acl_fiber_schedule_with(__event_type);
	printf("---thread-%lu, fiber consumers over, count=%d\r\n",
		pthread_self(), __count);
	return NULL;
}

static void *thread_alone_consumer(void *arg acl_unused)
{
	do_consume();
	printf("---thread-%lu, thread consumer over, count=%d\r\n",
		pthread_self(), __count);
	return NULL;
}

static void test(int nthreads, int nthreads2, unsigned flags)
{
	int i;
	pthread_t producers[nthreads], consumers[nthreads];
	pthread_t consumers_alone[nthreads2];
	struct timeval begin, end;
	double cost, speed;
	long long n;

	__mutex = acl_fiber_mutex_create(flags);
	__cond  = acl_fiber_cond_create(0);

	__atomic = acl_atomic_new();
	acl_atomic_set(__atomic, &__atomic_value);
	acl_atomic_int64_set(__atomic, 0);

	__atomic_signal = acl_atomic_new();
	acl_atomic_set(__atomic_signal, &__atomic_signal_value);
	acl_atomic_int64_set(__atomic_signal, 0);

	gettimeofday(&begin, NULL);

	for (i = 0; i < nthreads; i++) {
		pthread_create(&producers[i], NULL, thread_producer, NULL);
		printf("---create one producer--%lu\n", producers[i]);
	}

	for (i = 0; i < nthreads; i++) {
		pthread_create(&consumers[i], NULL, thread_consumer, NULL);
		printf("---create one consumer--%lu\n", consumers[i]);
	}

	for (i = 0; i < nthreads2; i++) {
		pthread_create(&consumers_alone[i], NULL,
			thread_alone_consumer, NULL);
		printf("---create one thread consumer--%lu\n",
			consumers_alone[i]);
	}

	for (i = 0; i < nthreads; i++) {
		pthread_join(consumers[i], NULL);
	}

	for (i = 0; i < nthreads2; i++) {
		pthread_join(consumers_alone[i], NULL);
	}

	__all_consumers_exit = 1;
	printf("---All consumer threads over---\r\n");

	for (i = 0; i < nthreads; i++) {
		pthread_join(producers[i], NULL);
	}
	printf("---All producer threads over---\r\n");

	gettimeofday(&end, NULL);

	n = acl_atomic_int64_fetch_add(__atomic, 0);
	cost = stamp_sub(&end, &begin);
	speed = (n * 1000) / (cost > 0 ? cost : 0.1);

	printf("---ALL OVER, count=%lld, cost=%.2f ms, speed=%.2f---\n",
		n, cost, speed);

	acl_fiber_mutex_free(__mutex);
	acl_fiber_cond_free(__cond);
	acl_atomic_free(__atomic);
	acl_atomic_free(__atomic_signal);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -e schedule_event_type[kernel|poll|select|io_uring]\r\n"
		" -t threads_count\r\n"
		" -c fibers_count\r\n"
		" -p threads_alone_count\r\n"
		" -n total_loop_count\r\n"
		" -r wait_timeout[default: -1]\r\n"
		" -D [if open debug log to stdout]\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	int  ch, nthreads = 1, nthreads2 = 0, debug = 0;
	unsigned flags = 0;

	while ((ch = getopt(argc, argv, "he:t:c:p:n:Tr:D")) > 0) {
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
			}
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
			__total_count = atoi(optarg);
			break;
		case 'r':
			__wait_timeout = atoi(optarg);
			break;
		case 'T':
			flags |= FIBER_MUTEX_F_LOCK_TRY;
			break;
		case 'D':
			debug = 1;
			break;
		default:
			break;
		}
	}

	if (debug) {
		acl_fiber_msg_stdout_enable(1);
	}

	test(nthreads, nthreads2, flags);
	return 0;
}
