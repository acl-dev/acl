#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <string.h>
#include <stdlib.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_malloc.h"
#include "malloc_vars.h"
#include "stdlib/acl_msg.h"

#endif

#include "../../private/thread.h"

static int  __debug_mem = 0;

static size_t  __alloc_max = 1024000;
static int *__alloc_stat;
static int  __alloc_over_1MB;

static acl_pthread_mutex_t __fastmutex_stat;

static size_t __malloc_length = 0;
static int __malloc_call_counter = 0;
static int __calloc_call_counter = 0;
static int __realloc_call_counter = 0;
static int __strdup_call_counter = 0;
static int __strndup_call_counter = 0;
static int __memdup_call_counter = 0;
static int __free_call_counter = 0;
static int __mem_counter = 0;
static int __mem_stack = 0;
/*----------------------------------------------------------------------------*/
/* debug memory and display memory status */

#define	MSTAT_LOCK	thread_mutex_lock(&__fastmutex_stat)
#define	MSTAT_UNLOCK	thread_mutex_unlock(&__fastmutex_stat)

void acl_memory_debug_start(void)
{
	__alloc_stat = calloc(__alloc_max, sizeof(int));
	acl_assert(__alloc_stat);
	thread_mutex_init(&__fastmutex_stat, NULL);
	__debug_mem = 1;
}

void acl_memory_debug_stop(void)
{
	if (__alloc_stat) {
		free(__alloc_stat);
		__alloc_stat = NULL;
		__debug_mem = 0;
		thread_mutex_destroy(&__fastmutex_stat);
	}
}

void acl_memory_debug_stack(int onoff)
{
	__mem_stack = onoff;
}

static void __mem_alloc_stat(void)
{
	size_t   i;

	if (__alloc_max == 0 || __alloc_stat == NULL)
		return;

	acl_msg_info("----------------alloc status---------------------------");
	for (i = 0; i < __alloc_max; i++) {
		if (__alloc_stat[i] > 0)
			acl_msg_info("%d byte: %d", (int) i, __alloc_stat[i]);
	}
}

void acl_memory_stat(void)
{
	int   n;

	if (!__debug_mem) {
		acl_msg_warn("Please call acl_memory_debug first!");
		return;
	}

	MSTAT_LOCK;
	acl_msg_info("-----------------mem status----------------------------");
	acl_msg_info("__malloc_call_counter = %d\r\n", __malloc_call_counter);
	acl_msg_info("__calloc_call_counter = %d\r\n", __calloc_call_counter);
	acl_msg_info("__realloc_call_counter = %d\r\n", __realloc_call_counter);
	acl_msg_info("__strdup_call_counter = %d\r\n", __strdup_call_counter);
	acl_msg_info("__strndup_call_counter = %d\r\n", __strndup_call_counter);
	acl_msg_info("__memdup_call_counter = %d\r\n", __memdup_call_counter);
	acl_msg_info("__free_call_counter = %d\r\n", __free_call_counter);
	acl_msg_info("__malloc_length = %u\r\n", (unsigned int) __malloc_length);
	n = __malloc_call_counter
		+ __calloc_call_counter
		+ __strdup_call_counter
		+ __strndup_call_counter
		+ __memdup_call_counter;
	acl_msg_info("total malloc = %d, total free = %d, inter = %d\r\n",
		n, __free_call_counter, n - __free_call_counter);
	acl_msg_info("---------------------------------------------------\r\n");
	MSTAT_UNLOCK;
}

void acl_memory_alloc_stat(void)
{
	__mem_alloc_stat();
}

/*----------------------------------------------------------------------------*/

void *(*__malloc_fn)(const char*, int, size_t) = acl_default_malloc;
void *(*__calloc_fn)(const char*, int, size_t, size_t) = acl_default_calloc;
void *(*__realloc_fn)(const char*, int, void*, size_t) = acl_default_realloc;
char *(*__strdup_fn)(const char*, int, const char*)  = acl_default_strdup;
char *(*__strndup_fn)(const char*, int, const char*, size_t) = acl_default_strndup;
void *(*__memdup_fn)(const char*, int, const void*, size_t) = acl_default_memdup;
void  (*__free_fn)(const char*, int, void*) = acl_default_free;

void *acl_malloc_glue(const char *filename, int line, size_t size)
{
	if (__alloc_stat) {
		if (size >= __alloc_max)
			__alloc_over_1MB++;
		else
			__alloc_stat[size]++;
	}

	if (__debug_mem) {
		MSTAT_LOCK;
		__malloc_call_counter++;
		__mem_counter++;
		__malloc_length += size;
		MSTAT_UNLOCK;
		if (__mem_stack)
			acl_msg_info("malloc: file=%s, line=%d",
				filename, line);
	}

	return (__malloc_fn(filename, line, size));
}

void *acl_calloc_glue(const char *filename, int line, size_t nmemb, size_t size)
{
	if (__alloc_stat) {
		if (size >= __alloc_max)
			__alloc_over_1MB++;
		else
			__alloc_stat[size]++;
	}

	if (__debug_mem) {
		MSTAT_LOCK;
		__calloc_call_counter++;
		__mem_counter++;
		__malloc_length += size;
		MSTAT_UNLOCK;
		if (__mem_stack)
			acl_msg_info("calloc: file=%s, line=%d",
				filename, line);
	}

	return (__calloc_fn(filename, line, nmemb, size));
}

void *acl_realloc_glue(const char *filename, int line, void *ptr, size_t size)
{
	if (__alloc_stat) {
		if (size >= __alloc_max)
			__alloc_over_1MB++;
		else
			__alloc_stat[size]++;
	}

	if (__debug_mem) {
		MSTAT_LOCK;
		__realloc_call_counter++;
		if (__realloc_fn == acl_default_realloc) {
			size_t   len;

			acl_default_memstat(filename, line, ptr, &len, NULL);
			__malloc_length -= len;
			__malloc_length += size;
		}
		MSTAT_UNLOCK;
		if (__mem_stack)
			acl_msg_info("realloc: file=%s, line=%d",
				filename, line);
	}

	return (__realloc_fn(filename, line, ptr, size));
}

char *acl_strdup_glue(const char *filename, int line, const char *str)
{
	if (__alloc_stat) {
		size_t  len = strlen(str);

		if (len >= __alloc_max)
			__alloc_over_1MB++;
		else
			__alloc_stat[len]++;
	}

	if (__debug_mem) {
		MSTAT_LOCK;
		__strdup_call_counter++;
		__mem_counter++;
		__malloc_length += strlen(str);
		MSTAT_UNLOCK;
		if (__mem_stack)
			acl_msg_info("strdup: file=%s, line=%d",
				filename, line);
	}

	return (__strdup_fn(filename, line, str));
}

char *acl_strndup_glue(const char *filename, int line,
	const char *str, size_t len)
{
	if (__alloc_stat) {
		size_t  n = strlen(str);

		n = n > len ? len : n;
		if (n >= __alloc_max)
			__alloc_over_1MB++;
		else
			__alloc_stat[n]++;
	}

	if (__debug_mem) {
		size_t   n = strlen(str);

		MSTAT_LOCK;
		__strndup_call_counter++;
		__mem_counter++;
		__malloc_length += len > n ? n: len;
		MSTAT_UNLOCK;
		if (__mem_stack)
			acl_msg_info("strndup: file=%s, line=%d",
				filename, line);
	}

	return (__strndup_fn(filename, line, str, len));
}

void *acl_memdup_glue(const char *filename, int line,
	const void *ptr, size_t len)
{
	if (__alloc_stat) {
		if (len >= __alloc_max)
			__alloc_over_1MB++;
		else
			__alloc_stat[len]++;
	}

	if (__debug_mem) {
		MSTAT_LOCK;
		__memdup_call_counter++;
		__mem_counter++;
		__malloc_length += len;
		MSTAT_UNLOCK;
		if (__mem_stack)
			acl_msg_info("memdup: file=%s, line=%d",
				filename, line);
	}

	return (__memdup_fn(filename, line, ptr, len));
}

void acl_free_glue(const char *filename, int line, void *ptr)
{
	if (__debug_mem) {
		MSTAT_LOCK;
		__free_call_counter++;
		__mem_counter--;
		if (__free_fn == acl_default_free) {
			size_t   len;

			acl_default_memstat(filename, line, ptr, &len, NULL);
			__malloc_length -= len;
		}
		MSTAT_UNLOCK;
		if (__mem_stack)
			acl_msg_info("free: file=%s, line=%d", filename, line);
	}

	__free_fn(filename, line, ptr);
}

void acl_free_fn_glue(void *ptr)
{
	if (__debug_mem) {
		MSTAT_LOCK;
		__free_call_counter++;
		__mem_counter--;
		if (__free_fn == acl_default_free) {
			size_t   len;

			acl_default_memstat("unknown", 0, ptr, &len, NULL);
			__malloc_length -= len;
		}
		MSTAT_UNLOCK;
	}

	__free_fn("unknown", 0, ptr);
}
