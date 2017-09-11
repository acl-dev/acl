#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_mem_hook.h"
#include <string.h>
#include <stdarg.h>
#include "stdlib/acl_malloc.h"
#include "stdlib/acl_mem_hook.h"

#endif

#include "malloc_vars.h"

/* xxx: 如果想要使用 pthread_spinlock_t 则不可将 stdlib.h 放在前面,
 * 否则编译报错
 */

#ifdef	ACL_UNIX
# ifndef  _GNU_SOURCE
#  define _GNU_SOURCE
# endif
# ifndef __USE_UNIX98
#  define __USE_UNIX98
# endif
# include <pthread.h>
#endif

#if defined(ACL_HAS_SPINLOCK) && !(MINGW)
static pthread_spinlock_t __lock;

#define	MUTEX_INIT	pthread_spin_init(&__lock, PTHREAD_PROCESS_PRIVATE)
#define	MUTEX_DESTROY	pthread_spin_destroy(&__lock)
#define	MUTEX_LOCK	pthread_spin_lock(&__lock)
#define	MUTEX_UNLOCK	pthread_spin_unlock(&__lock)

#else

#include "../../private/thread.h"

static acl_pthread_mutex_t __lock;

#define	MUTEX_INIT	thread_mutex_init(&__lock, NULL)
#define	MUTEX_DESTROY	thread_mutex_destroy(&__lock)
#define	MUTEX_LOCK	thread_mutex_lock(&__lock)
#define	MUTEX_UNLOCK	thread_mutex_unlock(&__lock)

#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_allocator.h"

static int __use_lock = 0;
static ACL_ALLOCATOR *__var_allocator = NULL;

static void *mempool_malloc(const char *filename, int line, size_t size)
{
	return (acl_allocator_membuf_alloc(filename, line, __var_allocator, size));
}

static void *mempool_calloc(const char *filename, int line, size_t nmemb, size_t size)
{
	void *ptr;

	ptr = acl_allocator_membuf_alloc(filename, line, __var_allocator, nmemb * size);
	memset(ptr, 0, nmemb * size);
	return (ptr);
}

static void *mempool_realloc(const char *filename, int line, void *ptr, size_t size)
{
	return (acl_allocator_membuf_realloc(filename, line, __var_allocator, ptr, size));
}

static char *mempool_strdup(const char *filename, int line, const char *str)
{
	char *ptr;
	size_t size = strlen(str) + 1;

	ptr = acl_allocator_membuf_alloc(filename, line, __var_allocator, size);
	strcpy(ptr, str);
	return (ptr);
}

static char *mempool_strndup(const char *filename, int line, const char *str, size_t len)
{
	char *ptr;
	const char *cp;

	if ((cp = memchr(str, 0, len)) != 0)
		len = cp - str;
	ptr = acl_allocator_membuf_alloc(filename, line, __var_allocator, len + 1);
	memcpy(ptr, str, len);
	ptr[len] = 0;

	return (ptr);
}

static void *mempool_memdup(const char *filename, int line, const void *ptr, size_t len)
{
	void *buf;

	buf = acl_allocator_membuf_alloc(filename, line, __var_allocator, len + 1);
	memcpy(buf, ptr, len);
	return (buf);
}

static void mempool_free(const char *filename, int line, void *ptr)
{
	acl_allocator_membuf_free(filename, line, __var_allocator, ptr);
}

static void *mempool_malloc_with_mutex(const char *filename, int line, size_t size)
{
	void *ptr;

	MUTEX_LOCK;
	ptr = acl_allocator_membuf_alloc(filename, line, __var_allocator, size);
	MUTEX_UNLOCK;

	return (ptr);
}

static void *mempool_calloc_with_mutex(const char *filename, int line, size_t nmemb, size_t size)
{
	void *ptr;

	MUTEX_LOCK;
	ptr = acl_allocator_membuf_alloc(filename, line, __var_allocator, nmemb * size);
	MUTEX_UNLOCK;

	memset(ptr, 0, nmemb * size);
	return (ptr);
}

static void *mempool_realloc_with_mutex(const char *filename, int line, void *ptr, size_t size)
{
	void *buf;

	MUTEX_LOCK;
	buf = acl_allocator_membuf_realloc(filename, line, __var_allocator, ptr, size);
	MUTEX_UNLOCK;

	return (buf);
}

static char *mempool_strdup_with_mutex(const char *filename, int line, const char *str)
{
	char *ptr;
	size_t size = strlen(str) + 1;

	MUTEX_LOCK;
	ptr = acl_allocator_membuf_alloc(filename, line, __var_allocator, size);
	MUTEX_UNLOCK;

	strcpy(ptr, str);
	return (ptr);
}

static char *mempool_strndup_with_mutex(const char *filename, int line, const char *str, size_t len)
{
	char *ptr;
	const char *cp;

	if ((cp = memchr(str, 0, len)) != 0)
		len = cp - str;
	MUTEX_LOCK;
	ptr = acl_allocator_membuf_alloc(filename, line, __var_allocator, len + 1);
	MUTEX_UNLOCK;
	memcpy(ptr, str, len);
	ptr[len] = 0;

	return (ptr);
}

static void *mempool_memdup_with_mutex(const char *filename, int line, const void *ptr, size_t len)
{
	void *buf;

	MUTEX_LOCK;
	buf = acl_allocator_membuf_alloc(filename, line, __var_allocator, len + 1);
	MUTEX_UNLOCK;

	memcpy(buf, ptr, len);
	return (buf);
}

static void mempool_free_with_mutex(const char *filename, int line, void *ptr)
{
	MUTEX_LOCK;
	acl_allocator_membuf_free(filename, line, __var_allocator, ptr);
	MUTEX_UNLOCK;
}

static void mempool_init_with_mutex(void)
{
	if (__use_lock == 0) {
		MUTEX_INIT;
		__use_lock = 1;
	}
	acl_mem_hook(mempool_malloc_with_mutex,
		mempool_calloc_with_mutex,
		mempool_realloc_with_mutex,
		mempool_strdup_with_mutex,
		mempool_strndup_with_mutex,
		mempool_memdup_with_mutex,
		mempool_free_with_mutex);
}

static void mempool_init_no_mutex(void)
{
	acl_mem_hook(mempool_malloc,
		mempool_calloc,
		mempool_realloc,
		mempool_strdup,
		mempool_strndup,
		mempool_memdup,
		mempool_free);
}

static void mempool_close(void)
{
	acl_mem_hook(acl_default_malloc,
		acl_default_calloc,
		acl_default_realloc,
		acl_default_strdup,
		acl_default_strndup,
		acl_default_memdup,
		acl_default_free);
}

void acl_mempool_open(size_t max_size, int use_mutex)
{
	if (__var_allocator == NULL)
		__var_allocator = acl_allocator_create(max_size);

	if (use_mutex)
		mempool_init_with_mutex();
	else
		mempool_init_no_mutex();
}

void acl_mempool_close(void)
{
	if (__use_lock) {
		MUTEX_DESTROY;
		__use_lock = 0;
	}

	if (__var_allocator != NULL) {
		acl_allocator_free(__var_allocator);
		__var_allocator = NULL;
	}

	mempool_close();
}

void acl_mempool_ctl(int name, ...)
{
	const char *myname = "acl_mempool_ctl";
	va_list ap;
	int   n;

	if (__var_allocator == NULL)
		acl_msg_fatal("%s(%d): call acl_mempool_create first", myname, __LINE__);

	va_start(ap, name);

	for (; name != ACL_MEMPOOL_CTL_END; name = va_arg(ap, int)) {
		switch (name) {
		case ACL_MEMPOOL_CTL_MUTEX:
			n = va_arg(ap, int);
			if (n)
				mempool_init_with_mutex();
			else
				mempool_init_no_mutex();
			break;
		case ACL_MEMPOOL_CTL_DISABLE:
			n = va_arg(ap, int);
			if (n)
				mempool_close();
			break;
		default:
			acl_msg_panic("%s: bad name %d", myname, name);
		}
	}
	va_end(ap);
}

int acl_mempool_total_allocated()
{
	int   n;

	if (__var_allocator == NULL)
		return (0);
	if (__use_lock)
		MUTEX_LOCK;
	n = acl_allocator_pool_total_allocated(__var_allocator);
	if (__use_lock)
		MUTEX_UNLOCK;

	return (n);
}
