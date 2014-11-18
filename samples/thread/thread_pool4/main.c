#include <getopt.h>
#include "lib_acl.h"

static int __count = 0;
static int __nsleep = 0;

static void thread_run(void *arg)
{
	acl_pthread_mutex_t * mutex = (acl_pthread_mutex_t*) arg;

	if (__nsleep) {
		printf(">>>thread id: %lu, sleep\r\n", acl_pthread_self());
		acl_doze(__nsleep);
	} else
		printf(">>>thread id: %lu, no sleep\r\n", acl_pthread_self());

	acl_pthread_mutex_lock(mutex);
	__count++;
	acl_pthread_mutex_unlock(mutex);
}

static int thread_init(void *arg acl_unused)
{
	printf(">>>thread init, id: %lu\r\n", acl_pthread_self());
	return 0;
}

static void thread_exit(void *arg acl_unused)
{
	printf(">>>thread exit, id: %lu\r\n", acl_pthread_self());
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help]\r\n"
		"	-n max_threads[default: 50]\r\n"
		"	-t thread_idle_timeout_seconds[default: 10]\r\n"
		"	-w schedule_wait_ms[default: 5000]\r\n",
		procname);
}

int main(int argc, char *argv[])
{
	int   nthreads = 50, i, ch;
	int   idle_ttl = 10 /* seconds */, schedule_wait = 5000 /* ms */;
	acl_pthread_pool_t *thrpool;
	acl_pthread_mutex_t mutex;

	while ((ch = getopt(argc, argv, "hn:w:t:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			nthreads = atoi(optarg);
			break;
		case 't':
			idle_ttl = atoi(optarg);
			break;
		case 'w':
			schedule_wait = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl_pthread_mutex_init(&mutex, NULL);
	thrpool = acl_thread_pool_create(nthreads, idle_ttl);

	acl_pthread_pool_atinit(thrpool, thread_init, NULL);
	acl_pthread_pool_atfree(thrpool, thread_exit, NULL);

	if (schedule_wait > 0)
		acl_pthread_pool_set_schedule_wait(thrpool, 5);

	printf("============ should use many threads ==================\r\n");
	printf("Enter any key to continue...");
	fflush(stdout);
	getchar();

	__nsleep = 100;
	__count = 0;

	for (i = 0; i < nthreads * 2; i++)
		acl_pthread_pool_add(thrpool, thread_run, &mutex);

	while (__count != nthreads * 2)
		acl_doze(50);
	acl_doze(50);

	printf("=========== should use only one thread ================\r\n");
	printf(">>>threads count: %d\r\n", acl_pthread_pool_size(thrpool));
	printf("Enter any key to continue ...");
	fflush(stdout);
	getchar();

	__nsleep = 0;
	__count = 0;

	for (i = 0; i < nthreads; i++) {
		acl_pthread_pool_add(thrpool, thread_run, &mutex);
		acl_doze(10);
	}

	while (__count != nthreads)
		acl_doze(50);
	acl_doze(50);

	printf("=========== should use many threads ===================\r\n");

	printf(">>>threads count: %d\r\n", acl_pthread_pool_size(thrpool));
	printf("Enter any key to continue ...");
	fflush(stdout);
	getchar();

	__count = 0;

	for (i = 0; i < nthreads; i++)
		acl_pthread_pool_add(thrpool, thread_run, &mutex);

	while (__count != nthreads)
		acl_doze(50);
	acl_doze(50);

	printf("=========== should use only one thread ================\r\n");

	printf(">>>threads count: %d\r\n", acl_pthread_pool_size(thrpool));
	printf("Enter any key to continue ...");
	fflush(stdout);
	getchar();

	__count = 0;

	for (i = 0; i < nthreads; i++) {
		acl_pthread_pool_add(thrpool, thread_run, &mutex);
		acl_doze(10);
	}

	printf("=======================================================\r\n");

	printf("enter any key to exit\r\n");
	getchar();
	acl_pthread_pool_destroy(thrpool);
	acl_pthread_mutex_destroy(&mutex);

	return 0;
}
