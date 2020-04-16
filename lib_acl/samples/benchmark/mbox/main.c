#include "lib_acl.h"
#include "../stamp.h"

static ACL_ATOMIC *__counter;
static unsigned __nproducers = 5;
static int __max = 10000;

static void* consumer(void *ctx)
{
	ACL_MBOX *box = (ACL_MBOX*) ctx;
	void *o;
	int succ, i;
	long long n, cnt = __max * __nproducers;

	for (i = 0; i < cnt; i++) {
		n = acl_atomic_int64_add_fetch(__counter, 1);

		if (n <= 10) {
			printf("thread: %ld, n=%lld\r\n", (long) acl_pthread_self(), n);
		}

		o = (void*) acl_mbox_read(box, 0, &succ);
		(void) o;
	}
	return NULL;
}

static void *producer(void *ctx)
{
	ACL_MBOX *box = (ACL_MBOX*) ctx;
	int i;

	for (i = 0; i <__max; i++) {
		int ret = acl_mbox_send(box, NULL);
		assert(ret == 0);
	}

	return NULL;
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -p nproducers[default: 5]\r\n"
		" -n max_loop_per_thread[default: 10000]\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	unsigned i;
	acl_pthread_t *producers, consumer_tid;
	acl_pthread_attr_t attr;
	static long long value = 0, counter;
	struct timeval begin, end;
	double spent, speed;
	ACL_MBOX *box;
	int ch, ret;

	while ((ch = getopt(argc, argv, "hp:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'p':
			__nproducers = (unsigned) atoi(optarg);
			break;
		case 'n':
			__max = (unsigned) atoi(optarg);
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	box       = acl_mbox_create();
	producers = (acl_pthread_t*)
		acl_mycalloc(__nproducers, sizeof(acl_pthread_t));
	__counter = acl_atomic_new();

	acl_atomic_set(__counter, &value);
	acl_atomic_int64_set(__counter, 0);

	acl_pthread_attr_init(&attr);

	gettimeofday(&begin, NULL);

	ret = acl_pthread_create(&consumer_tid, &attr, consumer, box);

	for (i = 0; i < __nproducers; i++) {
		ret = acl_pthread_create(&producers[i], &attr, producer, box);
		assert(ret == 0);
	}

	for (i = 0; i < __nproducers; i++) {
		acl_pthread_join(producers[i], NULL);
	}

	counter = __nproducers * __max;
	while (value < counter) {
		usleep(100);
	}

	gettimeofday(&end, NULL);
	spent = stamp_sub(&end, &begin);
	speed = (counter * 1000) / (spent >= 1.0 ? spent : 1.0);
	printf("counter=%lld, loop=%lld, spent=%.2f ms, speed=%.2f\r\n",
		value, counter, spent, speed);

	acl_atomic_free(__counter);
	acl_pthread_join(consumer_tid, NULL);
	acl_myfree(producers);
	acl_mbox_free(box, NULL);

	return 0;
}
