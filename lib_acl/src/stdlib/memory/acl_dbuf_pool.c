#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#ifdef	ACL_UNIX
#include <unistd.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_dbuf_pool.h"

#endif

typedef struct ACL_DBUF {
        void *buf;
        void *ptr;
        struct ACL_DBUF *next;
} ACL_DBUF;

struct ACL_DBUF_POOL {
        ACL_DBUF *head;
        int block_size;
};

ACL_DBUF_POOL *acl_dbuf_pool_create(int block_size)
{
#ifdef	USE_VALLOC
	ACL_DBUF_POOL *pool = (ACL_DBUF_POOL*) valloc(sizeof(ACL_DBUF_POOL));
	memset(pool, 0, sizeof(ACL_DBUF_POOL));
#else
	ACL_DBUF_POOL *pool = (ACL_DBUF_POOL*) acl_mycalloc(1,
			sizeof(ACL_DBUF_POOL));
#endif
	int   size, page_size;

#ifdef ACL_UNIX
	page_size = getpagesize();
#elif defined(WIN32)
	SYSTEM_INFO info;

	memset(&info, 0, sizeof(SYSTEM_INFO));
	GetSystemInfo(&info);
	page_size = info.dwPageSize;
	if (page_size <= 0)
		page_size = 4096;
#else
	page_size = 4096;
#endif

	size = (block_size / page_size) * page_size;
	if (size == 0)
		size = page_size;

	pool->block_size = size;
	pool->head = NULL;
	return pool;
}

void acl_dbuf_pool_destroy(ACL_DBUF_POOL *pool)
{
	ACL_DBUF *iter, *tmp;

	iter = pool->head;
	while (iter) {
		tmp = iter;
		iter = iter->next;
#ifdef	USE_VALLOC
		free(tmp->buf);
		free(tmp);
#else
		acl_myfree(tmp->buf);
		acl_myfree(tmp);
#endif
	}

#ifdef	USE_VALLOC
	free(pool);
#else
	acl_myfree(pool);
#endif
}

static ACL_DBUF *acl_dbuf_alloc(ACL_DBUF_POOL *pool, int length)
{
#ifdef	USE_VALLOC
	ACL_DBUF *dbuf = (ACL_DBUF*) valloc(sizeof(ACL_DBUF));
	memset(dbuf, 0, sizeof(ACL_DBUF));
#else
	ACL_DBUF *dbuf = (ACL_DBUF*) acl_mycalloc(1, sizeof(ACL_DBUF));
#endif
	dbuf->next = NULL;

#ifdef	USE_VALLOC
	dbuf->buf = dbuf->ptr = (void*) valloc(length);
#else
	dbuf->buf = dbuf->ptr = (void*) acl_mymalloc(length);
#endif
	if (pool->head == NULL) {
		pool->head = dbuf;
	} else {
		dbuf->next = pool->head;
		pool->head = dbuf;
	}
	return dbuf;
}

static int acl_dbuf_free(ACL_DBUF *dbuf)
{
	if (dbuf->ptr != dbuf->buf)
		return 0;

#ifdef	USE_VALLOC
	free(dbuf->ptr);
	free(dbuf);
#else
	acl_myfree(dbuf->ptr);
	acl_myfree(dbuf);
#endif
	return 1;
}

void acl_dbuf_pool_free(ACL_DBUF_POOL *pool, void *ptr, int length)
{
	ACL_DBUF *dbuf, *next;

	if (pool->head == NULL)
		return;

	dbuf = pool->head;
	if (ptr < dbuf->buf || (char*) dbuf->ptr != (char*) ptr + length) {
		return;
	}

	dbuf->ptr = ptr;

	if (length > pool->block_size) {
		next = dbuf->next;
		if (acl_dbuf_free(dbuf))
			pool->head = next;
	}
}

void *acl_dbuf_pool_alloc(ACL_DBUF_POOL *pool, int length)
{
	void *ptr;
	ACL_DBUF *dbuf;

	if (length > pool->block_size)
		dbuf = acl_dbuf_alloc(pool, length);
	else if (pool->head == NULL)
		dbuf = acl_dbuf_alloc(pool, pool->block_size);
	else if (pool->block_size - ((char*) pool->head->ptr
		- (char*) pool->head->buf) < length)
	{
		dbuf = acl_dbuf_alloc(pool, pool->block_size);
	}
	else
		dbuf = pool->head;

	ptr = dbuf->ptr;
	dbuf->ptr = (char*) dbuf->ptr + length;
	return ptr;
}

void *acl_dbuf_pool_calloc(ACL_DBUF_POOL *pool, int length)
{
	void *ptr;

	ptr = acl_dbuf_pool_alloc(pool, length);
	if (ptr)
		memset(ptr, 0, length);
	return ptr;
}

char *acl_dbuf_pool_strdup(ACL_DBUF_POOL *pool, const char *s)
{
	size_t  len = strlen(s);
	char *ptr = (char*) acl_dbuf_pool_alloc(pool, len + 1);

	memcpy(ptr, s, len);
	ptr[len] = 0;
	return ptr;
}

void *acl_dbuf_pool_memdup(ACL_DBUF_POOL *pool, const void *s, size_t len)
{
	void *ptr = acl_dbuf_pool_alloc(pool, len);

	memcpy(ptr, s, len);
	return ptr;
}

void acl_dbuf_pool_test(size_t max)
{
	ACL_DBUF_POOL *pool;
	size_t   i, n = 1000000, len, j, k;

	for (j = 0; j < max; j++) {
		printf("begin alloc, max: %d\n", (int) n);
		pool = acl_dbuf_pool_create(0);
		for (i = 0; i < n; i++) {
			k = i % 10;
			switch (k) {
			case 0:
				len = 1024;
				break;
			case 1:
				len = 1999;
				break;
			case 2:
				len = 999;
				break;
			case 3:
				len = 230;
				break;
			case 4:
				len = 199;
				break;
			case 5:
				len = 99;
				break;
			case 6:
				len = 19;
				break;
			case 7:
				len = 29;
				break;
			case 8:
				len = 9;
				break;
			case 9:
				len = 399;
				break;
			default:
				len = 88;
				break;
			}
			(void) acl_dbuf_pool_alloc(pool, len);
		}
		printf("alloc over now, sleep(10)\n");
		sleep(10);
		acl_dbuf_pool_destroy(pool);
	}
}
