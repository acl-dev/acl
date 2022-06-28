#include "lib_acl.h"
#include <assert.h>

static acl_pthread_mutex_t __mutex;
static int  __n = 0;
static void run_thread(void *arg acl_unused)
{
	acl_pthread_mutex_lock(&__mutex);
	__n++;
	acl_pthread_mutex_unlock(&__mutex);
}

static void test_thread_pool(int nthreads, int timeout, int nloop,
	acl_int64 nwait, acl_int64 nwarn)
{
	acl_pthread_pool_t *thr_pool;
	int   i;

	acl_pthread_mutex_init(&__mutex, NULL);
	thr_pool = acl_thread_pool_create(nthreads, timeout);
	acl_pthread_pool_set_schedule_warn(thr_pool, nwarn);
	acl_pthread_pool_set_schedule_wait(thr_pool, nwait);

	for (i = 0; i < nloop; i++)
		acl_pthread_pool_add_one(thr_pool, run_thread, NULL);

	printf("ok, nloop: %d\r\n", nloop);
	acl_pthread_pool_destroy(thr_pool);
	printf("destroy ok\r\n");
	acl_pthread_mutex_destroy(&__mutex);
	printf("at last n: %d\r\n", __n);
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help]\r\n"
		"	-t threads_count[default: 10]\r\n"
		"	-T timeout\r\n"
		"	-w thread wait timeout\r\n"
		"	-W thread warn timeout\r\n"
		"	-r [set random peek]\r\n"
		"	-n loop_count[default: 100000]\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch;
	int   nthreads = 10, nloop = 100000, timeout = 10;
	int   nwait = 10, nwarn = 50;

	while ((ch = getopt(argc, argv, "ht:n:T:w:W:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 't':
			nthreads = atoi(optarg);
			break;
		case 'n':
			nloop = atoi(optarg);
			break;
		case 'T':
			timeout  = atoi(optarg);
			break;
		case 'w':
			nwait = atoi(optarg);
			break;
		case 'W':
			nwarn = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl_msg_stdout_enable(1);
	test_thread_pool(nthreads, timeout, nloop, nwait, nwarn);
	return 0;
}
