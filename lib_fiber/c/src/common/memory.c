#include "stdafx.h"
#include "memory.h"

#ifdef	FIBER_STACK_GUARD

static size_t page_size(void)
{
	static __thread long pgsz = 0;

	if (pgsz == 0) {
		pgsz = sysconf(_SC_PAGE_SIZE);
		assert(pgsz > 0);
	}

	return (size_t) pgsz;
}

static size_t stack_size(size_t size)
{
	size_t pgsz = page_size(), sz;
	if (size < pgsz) {
		size = pgsz;
	}
	sz = (size + pgsz - 1) & ~(pgsz - 1);
	return sz;
}

void *stack_alloc(size_t size)
{
	int    ret;
	char  *ptr = NULL;
	size_t pgsz = page_size();

	size = stack_size(size);
	size += pgsz;

	ret = posix_memalign((void *) &ptr, pgsz, size);
	if (ret != 0) {
		msg_fatal("%s(%d), %s: posix_memalign error %s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
	}

	ret = mprotect(ptr, pgsz, PROT_NONE);
	if (ret != 0) {
		msg_fatal("%s(%d), %s: mprotect error=%s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
	}

	ptr += pgsz;
	return ptr;
}

void stack_free(void *ptr)
{
	int ret;
	size_t pgsz = page_size();

	ptr = (char *) ptr - pgsz;
	ret = mprotect(ptr, page_size(), PROT_READ|PROT_WRITE);
	if (ret != 0) {
		msg_fatal("%s(%d), %s: mprotect error=%s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
	}
	free(ptr);
}

#else

void *stack_alloc(size_t size)
{
	return malloc(size);
}

void stack_free(void *ptr)
{
	free(ptr);
}

#endif

void *stack_calloc(size_t size)
{
	void* ptr = stack_alloc(size);

	if (ptr) {
		memset(ptr, 0, size);
	}
	return ptr;
}
