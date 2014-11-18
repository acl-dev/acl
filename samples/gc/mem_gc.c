#include "gc.h"
#include "lib_acl.h"
#include "mem_gc.h"

/*
#define GC_LINUX_THREADS
#define	GC_DEBUG
*/

static void *malloc_hook(const char *fname acl_unused, int line acl_unused, size_t size)
{
	return (GC_MALLOC(size));
/*
	return (GC_MALLOC_ATOMIC(size));
*/
}

static void *calloc_hook(const char *fname, int line, size_t nmemb, size_t size)
{
	void *ptr = malloc_hook(fname, line, nmemb * size);

	memset(ptr, 0, nmemb * size);
	return (ptr);
}

static void *realloc_hook(const char *fname acl_unused, int line acl_unused, void *ptr, size_t size)
{
	return (GC_REALLOC(ptr, size));
}

static char *strdup_hook(const char *fname acl_unused, int line acl_unused, const char *ptr)
{
	return (GC_STRDUP(ptr));
}

static char *strndup_hook(const char *fname, int line, const char *s, size_t len)
{
	size_t n = strlen(s);
	char *ptr;

	n = n > len ? len : n;
	ptr = (char*) malloc_hook(fname, line, len);
	memcpy(ptr, s, n);
	ptr[n] = 0;
	return (ptr);
}

static void *memdup_hook(const char *fname, int line, const void *p, size_t size)
{
	void *ptr;

	ptr = malloc_hook(fname, line, size);
	memcpy(ptr, p, size);
	return (ptr);
}

static void free_hook(const char *fname acl_unused, int line acl_unused, void *ptr)
{
	GC_FREE(ptr);
}

void mem_gc_hook()
{
	GC_INIT();
	acl_mem_hook(malloc_hook, calloc_hook, realloc_hook,
		strdup_hook, strndup_hook, memdup_hook, free_hook);
}

