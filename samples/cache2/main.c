#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef WIN32
#define snprintf _snprintf
#endif

typedef struct {
	char *buf;
	int   len;
} MYOBJ;

static void free_fn(const ACL_CACHE2_INFO *info, void *arg)
{
	MYOBJ *o = (MYOBJ*) arg;

	printf("%s: when_timeout: %ld, now: %ld, len: %d, deleted\n",
		info->key, (long) info->when_timeout, (long) time(NULL), o->len);
	acl_myfree(o->buf);
	acl_myfree(o);
}

static MYOBJ *myobj_new(int len)
{
	MYOBJ *o = (MYOBJ*) acl_mymalloc(sizeof(MYOBJ));

	o->buf = (char*) acl_mymalloc(len <= 0 ? 100 : len);
	o->len = len;
	return (o);
}

static void walk_fn(ACL_CACHE2_INFO *info, void *arg acl_unused)
{
	MYOBJ *o = (MYOBJ*) info->value;

	printf("%s: size: %d; when_timeout: %ld\n", info->key, o->len, (long) info->when_timeout);
}

static void test1(int n, int timeout)
{
	ACL_CACHE2_INFO *info;
	ACL_CACHE2 *cache;
	char  key[32];
	MYOBJ *o;
	int   i;

	cache = acl_cache2_create(n, free_fn);

	for (i = 0; i < n + 5; i++) {
		o = myobj_new(i + 1);
		snprintf(key, sizeof(key), "key(%d)", i);
		assert(acl_cache2_enter(cache, key, o, timeout));
		printf("add one: %s\n", key);
		sleep(1);
	}

	printf("\nfirst walk cache, cache size: %d\n", acl_cache2_size(cache));
	acl_cache2_walk(cache, walk_fn, NULL);
	printf("\nfirst call acl_cache2_timeout, size: %d\n", acl_cache2_size(cache));
	acl_cache2_timeout(cache);
	printf(">>>after first acl_cache2_timeout, second walk cache, cache's size: %d\n", acl_cache2_size(cache));
	acl_cache2_walk(cache, walk_fn, NULL);

	printf("\n");
	i = 0;
	while (i++ < 5) {
		printf("slee one second, i=%d\n", i);
		sleep(1);
	}

	printf("\nsecond call acl_cache_timeout, size: %d\n", acl_cache2_size(cache));
	acl_cache2_timeout(cache);
	printf(">>>after second acl_cache_timeout, third walk_cache, cache's size: %d\n", acl_cache2_size(cache));
	acl_cache2_walk(cache, walk_fn, NULL);

	o = (MYOBJ*) acl_cache2_find(cache, "key(5)");
	if (o == NULL)
		printf("\n>>>key(5) not exist\n");
	else
		printf("\n>>>key(5): len: %d\n", o->len);
	info = acl_cache2_locate(cache, "key(11)");
	if (info == NULL)
		printf("\n>>>key(11) not exist\n");
	else {
		o = (MYOBJ*) info->value;
		printf("\n>>>key(11): len: %d, when_timeout: %ld\n", o->len, (long) info->when_timeout);
	}

	printf("\nfree cache, size: %d\n", acl_cache2_size(cache));
	acl_cache2_free(cache);
}

static void test2(int n, int timeout)
{
	ACL_CACHE2_INFO *info;
	ACL_CACHE2 *cache;
	char  key[32];
	MYOBJ *o;
	int   i;

	cache = acl_cache2_create(n, free_fn);

	for (i = 0; i < n + 5; i++) {
		o = myobj_new(i + 1);
		snprintf(key, sizeof(key), "key(%d)", i);
		info = (ACL_CACHE2_INFO*) acl_cache2_find(cache, key);
		if (info != NULL)
			printf("the key: %s exist\n", key);
		printf("add one: %s, timeout: %d\n", key, timeout);
		assert(acl_cache2_enter(cache, key, o, timeout));
	}

	printf(">>>call acl_cache2_walk\n");
	acl_cache2_walk(cache, walk_fn, NULL);
	acl_cache2_free(cache);
}

static void test3(int n, int timeout)
{
	ACL_CACHE2_INFO *info;
	ACL_CACHE2 *cache;
	char  key[32];
	MYOBJ *o;
	int   i;
	ACL_ITER iter;

	printf(">>> total add: %d\n", n * 2);
	cache = acl_cache2_create(n * 2, free_fn);

	for (i = 0; i < n; i++) {
		o = myobj_new(2 * i + 1);
		snprintf(key, sizeof(key), "key(%d)", 2 * i + 1);
		info = (ACL_CACHE2_INFO*) acl_cache2_find(cache, key);
		if (info != NULL)
			printf("the key: %s exist\n", key);
		printf("add one: %s, timeout: %d\n", key, timeout);
		assert(acl_cache2_enter(cache, key, o, timeout));

		o = myobj_new(2 * i + 2);
		snprintf(key, sizeof(key), "key(%d)", 2 * i + 2);
		info = (ACL_CACHE2_INFO*) acl_cache2_find(cache, key);
		if (info != NULL)
			printf("the key: %s exist\n", key);
		printf("add one: %s, timeout: %d\n", key, timeout);
		assert(acl_cache2_enter(cache, key, o, timeout));
	}

	printf(">>>>acl_foreach\n");
	acl_foreach(iter, cache) {
		o = (MYOBJ*) iter.data;
		info = (ACL_CACHE2_INFO*) acl_iter_info(iter, cache);
		printf(">>>len=%d, key=%s\n", o->len, info->key);
	}

	acl_cache2_free(cache);
}


static void usage(const char *procname)
{
	printf("usage: %s -h [help] -n max_size -t timeout -c cmd[test1|test2|test3]\n", procname);
}

int main(int argc, char *argv[])
{
	int   n = 100, ch, timeout = 1;
	char  cmd[256];

	ACL_SAFE_STRNCPY(cmd, "test3", sizeof(cmd));
	while ((ch = getopt(argc, argv, "hn:t:c:")) > 0) {
		switch (ch) {
			case 'h':
				usage(argv[0]);
				exit (0);
			case 'n':
				n = atoi(optarg);
				break;
			case 't':
				timeout = atoi(optarg);
				break;
			case 'c':
				ACL_SAFE_STRNCPY(cmd, optarg, sizeof(cmd));
				break;
			default:
				break;
		}
	}
	(void) acl_mem_slice_init(8, 10240, 100000,
		ACL_SLICE_FLAG_GC2 | ACL_SLICE_FLAG_RTGC_OFF);
	if (strcasecmp(cmd, "test1") == 0)
		test1(n, timeout);
	else if (strcasecmp(cmd, "test2") == 0)
		test2(n, timeout);
	else
		test3(n, timeout);
	acl_mem_slice_destroy();
#ifdef ACL_MS_WINDOWS
	printf("Enter any key to exit...\n");
	getchar();
#endif
	return (0);
}
