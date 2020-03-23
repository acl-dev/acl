#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "thread/acl_thread.h"
#include "stdlib/acl_mem_hook.h"
#include "stdlib/acl_debug_malloc.h"

#endif

#include "htable.h"

#ifndef ASSERT
#define ASSERT acl_assert
#endif

#ifdef ACL_WINDOWS
# define SANE_STRDUP _strdup
#else
# define SANE_STRDUP strdup
#endif

#define DLEN	256
#define SEP		'|'

struct ACL_DEBUG_MEM {
	DEBUG_HTABLE *table;
	acl_pthread_mutex_t lock;
	FILE *dump_fp;
};

static struct ACL_DEBUG_MEM *__debug_mem = NULL;

#define LOCK  do {  \
	acl_pthread_mutex_lock(&__debug_mem->lock);  \
} while(0)

#define UNLOCK do {  \
	acl_pthread_mutex_unlock(&__debug_mem->lock);  \
} while(0)

static void *acl_debug_malloc(const char *filename, int line, size_t size)
{
	char  key[256], *value;
	void *ptr;

	ASSERT(__debug_mem);

	ptr = malloc(size);
	ASSERT(ptr);
	snprintf(key, sizeof(key), "0x%p", ptr);
	value = (char*) malloc(DLEN);
	snprintf(value, DLEN, "%s%c%d%c%d", filename, SEP, line, SEP, (int) size);
	LOCK;
	ASSERT(debug_htable_enter(__debug_mem->table, key, value));
	UNLOCK;
	return (ptr);
}

static void *acl_debug_calloc(const char *filename, int line, size_t nmemb, size_t size)
{
	char  key[256], *value;
	void *ptr;

	ASSERT(__debug_mem);

	ptr = calloc(nmemb, size);
	ASSERT(ptr);
	snprintf(key, sizeof(key), "0x%p", ptr);
	value = (char*) malloc(DLEN);
	ASSERT(value);
	snprintf(value, DLEN, "%s%c%d%c%d", filename, SEP, line, SEP, (int) (nmemb * size));
	LOCK;
	ASSERT(debug_htable_enter(__debug_mem->table, key, value));
	UNLOCK;
	return (ptr);
}

static void *acl_debug_realloc(const char *filename, int line, void *old, size_t size)
{
	char  key[256], *value;
	void *ptr;

	ASSERT(__debug_mem);

	snprintf(key, sizeof(key), "0x%p", old);
	LOCK;
	value = (char*) debug_htable_find(__debug_mem->table, key);
	if (value == NULL) {
		fprintf(__debug_mem->dump_fp, "corrumpt%c%s%c%d\n", SEP, filename, SEP, line);
	} else {
		debug_htable_delete(__debug_mem->table, key, NULL);
		free(value);
	}
	UNLOCK;

	ptr = realloc(old, size);
	ASSERT(ptr);
	snprintf(key, sizeof(key), "0x%p", ptr);
	value = (char*) malloc(DLEN);
	ASSERT(value);
	snprintf(value, DLEN, "%s%c%d%c%d", filename, SEP, line, SEP, (int) size);
	LOCK;
	ASSERT(debug_htable_enter(__debug_mem->table, key, value));
	UNLOCK;
	return (ptr);
}

static char *acl_debug_strdup(const char *filename, int line, const char *str)
{
	char  key[256], *value;
	size_t size = strlen(str);
	char *ptr;

	ASSERT(__debug_mem);

	ptr = SANE_STRDUP(str);
	ASSERT(ptr);
	snprintf(key, sizeof(key), "0x%p", ptr);
	value = (char*) malloc(DLEN);
	ASSERT(value);
	snprintf(value, DLEN, "%s%c%d%c%d", filename, SEP, line, SEP, (int) size);
	LOCK;
	ASSERT(debug_htable_enter(__debug_mem->table, key, value));
	UNLOCK;
	return (ptr);
}

static char *acl_debug_strndup(const char *filename, int line, const char *str, size_t size)
{
	char  key[256], *value;
	char *ptr;
	size_t n = strlen(str);

	ASSERT(__debug_mem);

	size--;
	ASSERT(n > 0);
	n = n > size ? size : n;
	ptr = (char*) malloc(n + 1);
	ASSERT(ptr);
	memcpy(ptr, str, n);
	ptr[n] = 0;

	snprintf(key, sizeof(key), "0x%p", ptr);
	value = (char*) malloc(DLEN);
	ASSERT(value);
	snprintf(value, DLEN, "%s%c%d%c%d", filename, SEP, line, SEP, (int) n);
	LOCK;
	ASSERT(debug_htable_enter(__debug_mem->table, key, value));
	UNLOCK;
	return (ptr);
}

static void *acl_debug_memdup(const char *filename, int line, const void *data, size_t size)
{
	char  key[256], *value;
	void *ptr;

	ASSERT(__debug_mem);

	ptr = malloc(size);
	ASSERT(ptr);
	memcpy(ptr, data, size);

	snprintf(key, sizeof(key), "0x%p", ptr);
	value = (char*) malloc(DLEN);
	ASSERT(value);
	snprintf(value, DLEN, "%s%c%d%c%d", filename, SEP, line, SEP, (int) size);
	LOCK;
	ASSERT(debug_htable_enter(__debug_mem->table, key, value));
	UNLOCK;
	return (ptr);
}

static void acl_debug_free(const char *filename, int line, void *ptr)
{
	char  key[256], *value;

	if (ptr == NULL)
		return;
	ASSERT(__debug_mem);

	snprintf(key, sizeof(key), "0x%p", ptr);

	LOCK;
	value = (char*) debug_htable_find(__debug_mem->table, key);
	if (value == NULL) {
		fprintf(__debug_mem->dump_fp, "corrupt%c%s%c%d\n", SEP, filename, SEP, line);
	} else {
		debug_htable_delete(__debug_mem->table, key, NULL);
		free(value);
		free(ptr);
	}
	UNLOCK;
}

static void walk_fn(DEBUG_HTABLE_INFO *info, char *arg acl_unused)
{
	char *value = info->value;

	fprintf(__debug_mem->dump_fp, "leak%c%s\n", SEP, value);
}

#ifndef HAVE_NO_ATEXIT
static void __free_fn(char *value)
{
	free(value);
}
#endif

void acl_debug_dump(void)
{
	const char *myname = "acl_debug_dump";

	if (__debug_mem == NULL)
		return;

	fprintf(__debug_mem->dump_fp, "%s(%d): begin scan memory alloc and free\n", myname, __LINE__);
	fflush(__debug_mem->dump_fp);
	LOCK;
	debug_htable_walk(__debug_mem->table, walk_fn, NULL);
	UNLOCK;
	fprintf(__debug_mem->dump_fp, "%s(%d): scan memory finish\n", myname, __LINE__);
	fflush(__debug_mem->dump_fp);
}

#ifndef HAVE_NO_ATEXIT
static void debug_dump_atexit(void)
{
	if (__debug_mem == NULL)
		return;

	acl_debug_dump();
	debug_htable_free(__debug_mem->table, __free_fn);
	fclose(__debug_mem->dump_fp);
	acl_pthread_mutex_destroy(&__debug_mem->lock);
	free(__debug_mem);
	__debug_mem = NULL;
}
#endif

ACL_DEBUG_MEM *acl_debug_malloc_init(ACL_DEBUG_MEM *debug_mem_ptr, const char *dump_file)
{
	if (debug_mem_ptr != NULL) {
		__debug_mem = debug_mem_ptr;
	} else {
		ASSERT(dump_file && *dump_file);

		__debug_mem = (ACL_DEBUG_MEM*) calloc(1, sizeof(ACL_DEBUG_MEM));
		__debug_mem->dump_fp = fopen(dump_file, "wb+");
		ASSERT(__debug_mem->dump_fp);
		__debug_mem->table = debug_htable_create(1000);
		ASSERT(__debug_mem->table);
		acl_pthread_mutex_init(&__debug_mem->lock, NULL);
#ifndef HAVE_NO_ATEXIT
		atexit(debug_dump_atexit);
#endif
		fprintf(__debug_mem->dump_fp, "begin set mem_hook\n");
		fflush(__debug_mem->dump_fp);
	}

	acl_mem_hook(acl_debug_malloc,
		acl_debug_calloc,
		acl_debug_realloc,
		acl_debug_strdup,
		acl_debug_strndup,
		acl_debug_memdup,
		acl_debug_free);

	return (__debug_mem);
}
