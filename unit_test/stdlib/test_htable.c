
#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>

#include "test_stdlib.h"

#ifndef	TRUE
#define	TRUE	1
#endif
#ifndef	FALSE
#define	FALSE	0
#endif

static ACL_HASH_FN __get_hash_fn(const char *ptr)
{
	if (strcasecmp(ptr, "hash_crc32") == 0)
		return (acl_hash_crc32);
	else if (strcasecmp(ptr, "hash_bin") == 0) 
		return (acl_hash_bin);
	else if (strcasecmp(ptr, "hash_test") == 0) 
		return (acl_hash_test);
	else if (strcasecmp(ptr, "hash_func2") == 0) 
		return (acl_hash_func2);
	else if (strcasecmp(ptr, "hash_func3") == 0) 
		return (acl_hash_func3);
	else if (strcasecmp(ptr, "hash_func4") == 0) 
		return (acl_hash_func4);
	else if (strcasecmp(ptr, "hash_func5") == 0) 
		return (acl_hash_func5);
	else if (strcasecmp(ptr, "hash_func6") == 0)
		return (acl_hash_func6);
	else
		return (NULL);
}

/*---------------------------------------------------------------------------*/

static ACL_HTABLE *__table;

int test_htable_create(AUT_LINE *test_line, void *arg acl_unused)
{
	const char *ptr;
	ACL_ARGV *argv;
	char *pname, *pvalue;
	int   i;
	ACL_HASH_FN hash_fn;

	AUT_SET_STR(test_line, "hash_fn", ptr);

	hash_fn = __get_hash_fn(ptr);

	AUT_SET_STR(test_line, "table", ptr);
	printf("table=[%s]\n", ptr);

	__table = acl_htable_create(1, 0);
	acl_htable_ctl(__table,
			ACL_HTABLE_CTL_RWLOCK, FALSE,
			ACL_HTABLE_CTL_HASH_FN, hash_fn,
			ACL_HTABLE_CTL_END);

	argv = acl_argv_split(ptr, "; ");

	for (i = 0; i < acl_argv_size(argv); i++) {
		pname = acl_argv_index(argv, i);
		pvalue = strchr(pname, ':');
		if (pvalue == NULL) {
			printf("invalid for [%s]\n", pname);
			continue;
		}
		*pvalue++ = 0;
		if (*pvalue == 0) {
			printf("invalid for [%s]\n", pname);
			continue;
		}
		(void) acl_htable_enter(__table, pname, acl_mystrdup(pvalue));
	}

	acl_argv_free(argv);

	return (0);
}

int test_htable_stat(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	const char *myname = "test_htable_stat";

	if (__table == NULL) {
		printf("%s: __table null\n", myname);
		return (-1);
	}

	acl_htable_stat(__table);

	return (0);
}
int test_htable_free(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	if (__table == NULL) {
		printf("null hash table\n");
		return (-1);
	}

	return (0);
}

int test_htable_find(AUT_LINE *test_line, void *arg acl_unused)
{
	const char *myname = "test_htable_find";
	const char *ptr;
	char *value;

	AUT_SET_STR(test_line, "name", ptr);

	value = acl_htable_find(__table, ptr);
	if (value) {
		printf("%s: ok, find it, %s=%s\n", myname, ptr, value);
		return (0);
	} else {
		printf("%s: error, not found %s\n", myname, ptr);
		return (-1);
	}
}

/*---------------------------------------------------------------------------*/

typedef struct __HTABLE_RWLOCK_CTX {
	ACL_HTABLE *table;
	AUT_LINE *test_line;
} __HTABLE_RWLOCK_CTX;

typedef struct __THR_CTX {
	char  key[256];
} __THR_CTX;

static __HTABLE_RWLOCK_CTX __rwlock_ctx;

static void __htable_rwlock_fn(void *arg)
{
	const char *myname = "__htable_rwlock_fn";
	char  key[256];
	__HTABLE_RWLOCK_CTX *ctx = (__HTABLE_RWLOCK_CTX *) arg;
	__THR_CTX *thr_ctx = acl_mycalloc(1, sizeof(__THR_CTX));

#ifdef	LINUX2
	snprintf(key, sizeof(key), "%lu", pthread_self());
#elif	defined(SUNOS5)
	snprintf(key, sizeof(key), "%d", pthread_self());
#endif

/*
	if (acl_htable_find(ctx->table, key)) {
		printf("key(%s) exist\n", key);
		return;
	}
*/

	ACL_SAFE_STRNCPY(thr_ctx->key, key, sizeof(thr_ctx->key));
	if (acl_htable_enter_r(ctx->table, key, (char *) thr_ctx, NULL, NULL) == -1)
		acl_msg_fatal("insert into htable error(%s), key(%s)",
				strerror(errno), key);

	printf("search key: %s\n", key);
	thr_ctx = (__THR_CTX *) acl_htable_find(ctx->table, key);
	if (thr_ctx == NULL)
		acl_msg_fatal("%s: not find key(%s)", myname, key);
	printf("ok, add and find, key=%s\n", key);
}

int test_htable_rwlock(AUT_LINE *test_line, void *arg acl_unused)
{
	acl_pthread_pool_t *tp;
	int  threads = 100, thread_idle = 120;
	int   i, n = 1000;
	ACL_HASH_FN hash_fn;
	const char *ptr;

	AUT_SET_STR(test_line, "hash_fn", ptr);

	hash_fn = __get_hash_fn(ptr);

	__rwlock_ctx.test_line = test_line;
	__rwlock_ctx.table = acl_htable_create(10, 0);

	acl_htable_ctl(__rwlock_ctx.table,
			ACL_HTABLE_CTL_RWLOCK, TRUE,
			ACL_HTABLE_CTL_HASH_FN, hash_fn,
			ACL_HTABLE_CTL_END);

	tp = acl_thread_pool_create(threads, thread_idle);

	for (i = 0; i < n; i++) {
		acl_pthread_pool_add(tp, __htable_rwlock_fn, &__rwlock_ctx);
	}

	sleep(2);

	acl_pthread_pool_stop(tp);
	acl_htable_stat(__rwlock_ctx.table);

	acl_htable_free(__rwlock_ctx.table, (void (*) (void *))acl_myfree_fn);
	__rwlock_ctx.test_line = NULL;
	__rwlock_ctx.table = NULL;

	return (0);
}
