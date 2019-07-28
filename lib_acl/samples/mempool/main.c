#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef WIN32
#pragma comment(lib,"ws2_32")
#endif

#if 1
#define	USE_ACL_MALLOC
#endif

#ifdef	USE_ACL_MALLOC
# define	MALLOC	acl_mymalloc
# define	FREE	acl_myfree
#else
# define	MALLOC	malloc
# define	FREE	free
#endif

#define	MEM_TYPE_GROSS	ACL_MEM_TYPE_MAX + 1
static ACL_ALLOCATOR *__var_allocator = NULL;
static size_t __max_size = 1024 * 1024 * 100;

static void init(void)
{
	__var_allocator = acl_allocator_create(__max_size);
}

static void end_prompt(void)
{
#ifdef	ACL_MS_WINDOWS
	printf("enter any key to continue.\r\n");
	getchar();
#endif
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -c bench|bench2|bench3\r\n", procname);
#ifdef	ACL_MS_WINDOWS
	getchar();
#endif
}

static void mempool_bench_test2(const char *label, int use_pool, int mutex, int loop, int size)
{
	char *buf;
	time_t begin, end;
	int   i;
	static int __pool_inited = 0;

	if (use_pool) {
		if (__pool_inited == 0) {
			acl_mempool_open(__max_size, mutex);
			__pool_inited = 1;
		} else {
			acl_mempool_ctl(ACL_MEMPOOL_CTL_MUTEX, mutex,
					ACL_MEMPOOL_CTL_END);
		}

		i = 0;
		time(&begin);
		while (i++ < loop) {
			buf = MALLOC(size);
			FREE(buf);
		}
		time(&end);
	} else {
		acl_pthread_mutex_t lock;

		if (mutex)
			acl_pthread_mutex_init(&lock, NULL);

		if (__pool_inited) {
			acl_mempool_ctl(ACL_MEMPOOL_CTL_DISABLE, 1,
					ACL_MEMPOOL_CTL_END);
		}

		i = 0;
		time(&begin);
		while (i++ < loop) {
			buf = MALLOC(size);
			FREE(buf);
		}
		time(&end);

		if (mutex)
			acl_pthread_mutex_destroy(&lock);
	}

	if (use_pool)
		printf("%s: time cost is %ld seconds, count is %d, total pool alloc %d\r\n",
			label, (long int) end - begin, loop, acl_mempool_total_allocated());
	else
		printf("%s: time cost is %ld seconds, count is %d\r\n",
			label, (long int) end - begin, loop);
}

struct MEM_POOL_BENCH2 {
	char *label;
	int   use_pool;
	int   mutex;
	int   loop;
	int   size;
};

#define	MAX_LOOP	10000000

static struct MEM_POOL_BENCH2 __pool_bench_tab2[] = {
	{ "alloc 64, pool, mutex", 1, 1, MAX_LOOP, 64 },
	{ "alloc 64, pool, no mutex", 1, 0, MAX_LOOP, 64 },
	{ "alloc 64, no pool, no mutex", 0, 0, MAX_LOOP, 64 },

	{ "alloc 2k, pool, mutex", 1, 1, MAX_LOOP, 1024 * 2 },
	{ "alloc 2k, pool, no mutex", 1, 0, MAX_LOOP, 1024 * 2 },
	{ "alloc 2k, no pool, no mutex", 0, 0, MAX_LOOP, 1024 * 2 },

	{ "alloc 8k, pool, mutex", 1, 1, MAX_LOOP, 1024 * 8 },
	{ "alloc 8k, pool, no mutex", 1, 0, MAX_LOOP, 1024 * 8 },
	{ "alloc 8k, no pool, no mutex", 0, 0, MAX_LOOP, 1024 * 8 },

	{ "alloc 64k, pool, mutex", 1, 1, MAX_LOOP, 1024 * 64 },
	{ "alloc 64k, pool, no mutex", 1, 0, MAX_LOOP, 1024 * 64 },
	{ "alloc 64k, no pool, no mutex", 0, 0, MAX_LOOP, 1024 * 64 },

	{ 0, 0, 0, 0, 0 },
};

static void mem_bench2(void)
{
	int   i;

	for (i = 0; __pool_bench_tab2[i].label != NULL; i++) {
		if (i % 3 == 0)
			printf("\r\n");
		mempool_bench_test2(__pool_bench_tab2[i].label,
				__pool_bench_tab2[i].use_pool,
				__pool_bench_tab2[i].mutex,
				__pool_bench_tab2[i].loop,
				__pool_bench_tab2[i].size);
	}
	end_prompt();
}

static struct MEM_POOL_BENCH2 __pool_bench_tab3[] = {
	{ "alloc 64, pool, mutex", 1, 1, MAX_LOOP, 64 },
	{ "alloc 64, pool, mutex", 1, 1, MAX_LOOP, 64 },
	{ "alloc 64, pool, mutex", 1, 1, MAX_LOOP, 64 },
	{ "alloc 64, pool, mutex", 1, 1, MAX_LOOP, 64 },
	{ 0, 0, 0, 0, 0 },
};

static void mem_bench3(void)
{
	int   i;

	for (i = 0; __pool_bench_tab3[i].label != NULL; i++) {
		if (i % 3 == 0)
			printf("\r\n");
		mempool_bench_test2(__pool_bench_tab3[i].label,
				__pool_bench_tab3[i].use_pool,
				__pool_bench_tab3[i].mutex,
				__pool_bench_tab3[i].loop,
				__pool_bench_tab3[i].size);
	}
	end_prompt();
}

static void mempool_bench_test(const char *label, int mutex, int loop, acl_mem_type type, int size)
{
	int   i = 0;
	time_t begin = 0, end = 0;
	void *buf;
#ifdef	MUTEX_INIT
	acl_pthread_mutex_t lock;
#elif defined(WIN32)
	acl_pthread_mutex_t lock;
#define MUTEX_INIT
#else
	acl_pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
#endif

	if (mutex) {
#ifdef	MUTEX_INIT
		acl_pthread_mutex_init(&lock, NULL);
#endif
		time(&begin);
		while (i++ < loop) {
			acl_pthread_mutex_lock(&lock);
			acl_pthread_mutex_unlock(&lock);
		}
		time(&end);
		printf("lock and unkock, loop %d, time cost is %ld\r\n", loop, (long int) end - begin);

		i = 0;
		if (type > ACL_MEM_TYPE_NONE && type < ACL_MEM_TYPE_MAX) {
			time(&begin);
			while (i++ < loop) {
				acl_pthread_mutex_lock(&lock);
				buf = acl_allocator_mem_alloc(__var_allocator, type);
				acl_pthread_mutex_unlock(&lock);

				acl_pthread_mutex_lock(&lock);
				acl_allocator_mem_free(__var_allocator, type, buf);
				acl_pthread_mutex_unlock(&lock);
			}
			time(&end);
		} else if (type == MEM_TYPE_GROSS) {
			acl_pthread_mutex_lock(&lock);
			buf = acl_allocator_membuf_alloc(__FILE__, __LINE__,
				__var_allocator, size);
			acl_pthread_mutex_unlock(&lock);

			acl_pthread_mutex_lock(&lock);
			acl_allocator_membuf_free(__FILE__, __LINE__,
				__var_allocator, buf);
			acl_pthread_mutex_unlock(&lock);
		} else {
			time(&begin);
			while (i++ < loop) {
				acl_pthread_mutex_lock(&lock);
				buf = MALLOC(size);
				acl_pthread_mutex_unlock(&lock);

				acl_pthread_mutex_lock(&lock);
				FREE(buf);
				acl_pthread_mutex_unlock(&lock);
			}
			time(&end);
		}
#ifdef	MUTEX_INIT
		acl_pthread_mutex_destroy(&lock);
#endif
	} else {
		if (type > ACL_MEM_TYPE_NONE && type < ACL_MEM_TYPE_MAX) {
			time(&begin);
			while (i++ < loop) {
				buf = acl_allocator_mem_alloc(__var_allocator, type);
				acl_allocator_mem_free(__var_allocator, type, buf);
			}
			time(&end);
		} else if (type == MEM_TYPE_GROSS) {
			buf = acl_allocator_membuf_alloc(__FILE__, __LINE__,
				__var_allocator, size);
			acl_allocator_membuf_free(__FILE__, __LINE__,
				__var_allocator, buf);
		} else {
			time(&begin);
			while (i++ < loop) {
				buf = MALLOC(size);
				FREE(buf);
			}
			time(&end);
		}
	}

	printf("%s: time cost is %ld seconds, count is %d\r\n",
		label, (long int) end - begin, loop);
}

struct MEM_POOL_BENCH {
	char *label;
	int   mutex;
	int   loop;
	acl_mem_type type;
	int   size;
};

#define	MAX_LOOP	10000000

static struct MEM_POOL_BENCH __pool_bench_tab[] = {
	{ "alloc vstring, pool, mutex", 1, MAX_LOOP, ACL_MEM_TYPE_VSTRING, 0 },
	{ "alloc vstring, pool, no mutex", 0, MAX_LOOP, ACL_MEM_TYPE_VSTRING, 0 },
	{ "alloc vstring, no pool, no mutex", 0, MAX_LOOP, ACL_MEM_TYPE_NONE, 1024 },

	{ "alloc 64, pool, mutex", 1, MAX_LOOP, ACL_MEM_TYPE_64_BUF, 0 },
	{ "alloc 64, pool, no mutex", 0, MAX_LOOP, ACL_MEM_TYPE_64_BUF, 0 },
	{ "alloc 64, no pool, no mutex", 0, MAX_LOOP, ACL_MEM_TYPE_NONE, 64 },

	{ "alloc 2K, pool, mutex", 1, MAX_LOOP, ACL_MEM_TYPE_2K_BUF, 0 },
	{ "alloc 2K, pool, no mutex", 0, MAX_LOOP, ACL_MEM_TYPE_2K_BUF, 0 },
	{ "alloc 2K, no pool, no mutex", 0, MAX_LOOP, ACL_MEM_TYPE_NONE, 1024 * 2 },

	{ "alloc 8K, pool, mutex", 1, MAX_LOOP, ACL_MEM_TYPE_8K_BUF, 0 },
	{ "alloc 8K, pool, no mutex", 0, MAX_LOOP, ACL_MEM_TYPE_8K_BUF, 0 },
	{ "alloc 8K, no pool, no mutex", 0, MAX_LOOP, ACL_MEM_TYPE_NONE, 1024 * 8 },

	{ "alloc 64K, pool, mutex", 1, MAX_LOOP, ACL_MEM_TYPE_64K_BUF, 0 },
	{ "alloc 64K, pool, no mutex", 0, MAX_LOOP, ACL_MEM_TYPE_64K_BUF, 0 },
	{ "alloc 64K, no pool, no mutex", 0, MAX_LOOP, ACL_MEM_TYPE_NONE, 1024 * 64 },

	{ 0, 0, 0, 0, 0 },
};

static void mem_bench(void)
{
	int   i;

	for (i = 0; __pool_bench_tab[i].label != NULL; i++) {
		if (i % 3 == 0)
			printf("\r\n");
		mempool_bench_test(__pool_bench_tab[i].label,
				__pool_bench_tab[i].mutex,
				__pool_bench_tab[i].loop,
				__pool_bench_tab[i].type,
				__pool_bench_tab[i].size);
	}

	end_prompt();
}

int main(int argc, char *argv[])
{
	char  ch;

	init();

	while ((ch = getopt(argc, argv, "hc:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			exit (0);
		case 'c':
			if (strcasecmp(optarg, "bench") == 0)
				mem_bench();
			else if (strcasecmp(optarg, "bench2") == 0)
				mem_bench2();
			else if (strcasecmp(optarg, "bench3") == 0)
				mem_bench3();
			else
				usage(argv[0]);
			exit (0);
		default:
			usage(argv[0]);
			exit (0);
		}
	}

	usage(argv[0]);
	exit (0);
}
