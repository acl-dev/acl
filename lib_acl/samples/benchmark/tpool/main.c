#include "lib_acl.h"
#include "../stamp.h"

static ACL_ATOMIC *__counter;
static int __max = 10000;

static void consumer(void *ctx acl_unused)
{
	long long n = acl_atomic_int64_add_fetch(__counter, 1);
	if (n <= 10) {
		printf("thread: %ld, n=%lld\r\n", (long) acl_pthread_self(), n);
	}
}

static void *producer(void *ctx)
{
	acl_pthread_pool_t *tpool = (acl_pthread_pool_t*) ctx;
	int i;

	for (i = 0; i <__max; i++) {
		acl_pthread_pool_add_one(tpool, consumer, tpool);
	}

	return NULL;
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -c nconsumers[default: 20]\r\n"
		" -p nproducers[default: 5]\r\n"
		" -n max_loop_per_thread[default: 10000]\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	unsigned nconsumers = 20, nproducers = 5, i, qsize = 0;
	acl_pthread_pool_t *tpool;
	acl_pthread_attr_t  attr;
	acl_pthread_t      *producers;
	struct timeval begin, end;
	double spent, speed;
	static long long value = 0, counter;
	int ch, debug = 0;

	(void) qsize;

	while ((ch = getopt(argc, argv, "hdq:c:p:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'd':
			debug = 1;
			break;
		case 'q':
			qsize = (unsigned) atoi(optarg);
			break;
		case 'c':
			nconsumers = (unsigned) atoi(optarg);
			break;
		case 'p':
			nproducers = (unsigned) atoi(optarg);
			break;
		case 'n':
			__max = (unsigned) atoi(optarg);
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	if (debug) {
		acl_msg_open("tpool.log", "tpool");
	}
	tpool     = acl_thread_pool_create(nconsumers, 0);
	producers = (acl_pthread_t*) acl_mycalloc(nproducers, sizeof(acl_pthread_t));
	__counter = acl_atomic_new();

	acl_atomic_set(__counter, &value);
	acl_atomic_int64_set(__counter, 0);

	acl_pthread_attr_init(&attr);

	gettimeofday(&begin, NULL);

	for (i = 0; i < nproducers; i++) {
		int ret = acl_pthread_create(&producers[i], &attr, producer, tpool);
		assert(ret == 0);
	}

	for (i = 0; i < nproducers; i++) {
		acl_pthread_join(producers[i], NULL);
	}

	counter = nproducers * __max;
	while (value < counter) {
		usleep(100);
	}

	gettimeofday(&end, NULL);
	spent = stamp_sub(&end, &begin);
	speed = (counter * 1000) / (spent >= 1.0 ? spent : 1.0);

	printf("counter=%lld, loop=%lld, spent=%.2f ms, speed=%.2f\r\n",
		value, counter, spent, speed);

	acl_atomic_free(__counter);
	acl_pthread_pool_destroy(tpool);
	acl_myfree(producers);

	if (debug) {
		acl_msg_close();
	}
	return 0;
}
