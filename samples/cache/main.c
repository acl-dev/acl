#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct {
	char *buf;
	int   len;
} MYOBJ;

static void free_fn(const ACL_CACHE_INFO *info, void *arg)
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

static void walk_fn(ACL_CACHE_INFO *info, void *arg acl_unused)
{
	MYOBJ *o = (MYOBJ*) info->value;

	printf("%s: size: %d; when_timeout: %ld\n", info->key, o->len, (long) info->when_timeout);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -n max_size -t timeout\n", procname);
}

int main(int argc, char *argv[])
{
	int   i, n = 100, ch, timeout = 1;
	ACL_CACHE_INFO *info;
	ACL_CACHE *cache;
	char  key[32];
	MYOBJ *o;

	while ((ch = getopt(argc, argv, "hn:t:")) > 0) {
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
			default:
				break;
		}
	}
	
	cache = acl_cache_create(n, timeout, free_fn);

	for (i = 0; i < n + 5; i++) {
		o = myobj_new(i + 1);
		snprintf(key, sizeof(key), "key(%d)", i);
		assert(acl_cache_enter(cache, key, o));
		printf("add one: %s\n", key);
		sleep(1);
	}

	printf("\nfirst walk cache, cache size: %d\n", acl_cache_size(cache));
	acl_cache_walk(cache, walk_fn, NULL);
	printf("\nfirst call acl_cache_timeout, size: %d\n", acl_cache_size(cache));
	acl_cache_timeout(cache);
	printf(">>>after first acl_cache_timeout, second walk cache, cache's size: %d\n", acl_cache_size(cache));
	acl_cache_walk(cache, walk_fn, NULL);

	printf("\n");
	i = 0;
	while (i++ < 5) {
		printf("slee one second, i=%d\n", i);
		sleep(1);
	}

	printf("\nsecond call acl_cache_timeout, size: %d\n", acl_cache_size(cache));
	acl_cache_timeout(cache);
	printf(">>>after second acl_cache_timeout, third walk_cache, cache's size: %d\n", acl_cache_size(cache));
	acl_cache_walk(cache, walk_fn, NULL);

	o = (MYOBJ*) acl_cache_find(cache, "key(5)");
	if (o == NULL)
		printf("\n>>>key(5) not exist\n");
	else
		printf("\n>>>key(5): len: %d\n", o->len);
	info = acl_cache_locate(cache, "key(11)");
	if (info == NULL)
		printf("\n>>>key(11) not exist\n");
	else {
		o = (MYOBJ*) info->value;
		printf("\n>>>key(11): len: %d, when_timeout: %ld\n", o->len, (long) info->when_timeout);
	}

	printf("\nfree cache, size: %d\n", acl_cache_size(cache));
	acl_cache_free(cache);
	return (0);
}
