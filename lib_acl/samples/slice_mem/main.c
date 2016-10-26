#include "lib_acl.h"
#include <time.h>

/*----------------------------------------------------------------------------*/

/* 线程池中某线程的任务处理过程 */

static void run_thread(void *arg)
{
	/* 释放主线程分配的内存 */
	if (1)
	acl_myfree(arg);
}

static void thread_pool_tls(int nthread, int nalloc)
{
	const char *myname = "thread_pool_tls";
	acl_pthread_pool_t *thr_pool;
	void *ptr;
	time_t begin = time(NULL);
	int   i;

	/* 创建线程池 */
	thr_pool = acl_thread_pool_create(nthread, 0);

	for (i = 0; i < nalloc; i++) {
		/* 主线程分配内存 */
		if (1)
		ptr = acl_mymalloc(64);
		else
		ptr = 0;
		/* 向线程池中添加任务, 将新内存传递给线程池处理 */
		/*
		 * ACL_METER_TIME("--31--");
		 */
		acl_pthread_pool_add(thr_pool, run_thread, ptr);
		if (i % 100000 == 0)
			printf(">>>%s: i=%d\n", myname, i);
		/*
		 * ACL_METER_TIME("--32--");
		 */
	}

	/* 释放线程池 */
	acl_pthread_pool_destroy(thr_pool);
	printf(">>>%s: time cost: %ld\n", myname, (long) (time(NULL) - begin));
}

/*----------------------------------------------------------------------------*/

static int __use_base = 0;

static void *test_tls_thread(void *arg acl_unused)
{
#define	MAX	30
	const char *myname = "test_tls_thread";
	void *ptr[MAX];
	time_t begin;
	int   i, j, k, *nalloc = (int*) arg, nloop1, nloop2, n;

	n = *nalloc < 10000 ? 10000 : *nalloc;
	nloop1 = 1000;
	nloop2 = n / nloop1;

	time(&begin);
	if (__use_base) {
		for (j = 0; j < nloop1; j++) {
			for (i = 0; i < nloop2; i++) {
				for (k = 0; k < MAX; k++)
					ptr[k] = malloc(128);
				for (k = 0; k < MAX; k++)
					free(ptr[k]);
			}
		}
	} else {
		for (j = 0; j < nloop1; j++) {
			for (i = 0; i < nloop2; i++) {
				for (k = 0; k < MAX; k++)
					ptr[k] = acl_mymalloc(128);
				for (k = 0; k < MAX; k++)
					acl_myfree(ptr[k]);
			}
		}
	}
	printf(">>>%s(%d)(tid=%ld): time cose %d seconds\n",
		myname, __LINE__, (long) acl_pthread_self(),
		(int) (time(NULL) - begin));
	return (NULL);
}

static void test_tls(int nthread, int nalloc)
{
	acl_pthread_attr_t attr;
	acl_pthread_t *tids;
	int   i;

	printf(">>> test_tls: begin running(thread=%ld) ...\n",
		(long) acl_pthread_self());
	tids = (acl_pthread_t*) acl_mycalloc(nthread, sizeof(acl_pthread_t));

	acl_pthread_attr_init(&attr);
	for (i = 0; i < nthread; i++) {
		acl_pthread_create(&tids[i], &attr, test_tls_thread, &nalloc);
	}
	for (i = 0; i < nthread; i++) {
		acl_pthread_join(tids[i], NULL);
		printf(">>> test_tls: thread(%ld) over now\n", (long) tids[i]);
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -s[use slice] -t nthread -n nalloc -g nalloc_gc -b[use malloc/free] -p[use thread pool]\n", procname);
}

int main(int argc, char *argv[])
{
	int base = 8, nslice = 1024, nalloc_gc = 1000000, use_thrpool = 0;
	int use_slice = 0, nthread = 2, nalloc = 10000000;
	unsigned int slice_flag = ACL_SLICE_FLAG_GC2 | ACL_SLICE_FLAG_RTGC_OFF;
	char  ch, *ptr;

	while ((ch = getopt(argc, argv, "hst:n:g:bp")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 's':
			use_slice = 1;
			break;
		case 't':
			nthread = atoi(optarg);
			if (nthread < 0)
				nthread = 2;
			break;
		case 'n':
			nalloc = atoi(optarg);
			if (nalloc < 100)
				nalloc = 100;
			break;
		case 'g':
			nalloc_gc = atoi(optarg);
			if (nalloc_gc < 100)
				nalloc_gc = 100;
			break;
		case 'b':
			__use_base = 1;
			break;
		case 'p':
			use_thrpool = 1;
			break;
		default:
			break;
		}
	}

	acl_lib_init();
	if (use_slice)
		acl_mem_slice_init(base, nslice, nalloc_gc, slice_flag);
	if (use_thrpool)
		thread_pool_tls(nthread, nalloc);
	else
		test_tls(nthread, nalloc);
	if (use_slice) {
		acl_mem_slice_destroy();

		/* 取消内存管理的勾子函数，恢复为缺省状态 */
		acl_mem_unhook();
	}

	/* 缺省的内存分配与释放 */

	ptr = acl_mymalloc(128);
	acl_myfree(ptr);

	printf("Input any key to exit ...\n");
	getchar();
	return (0);
}

