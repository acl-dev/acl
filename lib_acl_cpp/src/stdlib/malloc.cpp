#include "acl_stdafx.hpp"
#include <new>
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/malloc.hpp"
#endif

#ifdef ACL_HOOK_NEW

# ifdef ACL_WINDOWS

#  ifdef NDEBUG
void* operator new(size_t n)
{
	return acl::acl_new(n, __FILE__, __FUNCTION__, __LINE__);
}

void  operator delete(void *p)
{
	acl::acl_delete(p, __FILE__, __FUNCTION__, __LINE__);
}
#  endif

# else  // ACL_WINDOWS

void* operator new(size_t n) throw (std::bad_alloc)
{
	return acl::acl_new(n, __FILE__, __FUNCTION__, __LINE__);
}
void  operator delete(void *p) throw()
{
	acl::acl_delete(p, __FILE__, __FUNCTION__, __LINE__);
}

# endif  // !ACL_WINDOWS

#elif defined(ACL_CPP_DEBUG_MEM)

static ACL_HTABLE* __addrs = NULL;
static ACL_HTABLE* __mapper = NULL;
static acl_pthread_mutex_t* __lock = NULL;

void* operator new(size_t size, const char* file, const char* func,
	int line) throw()
{

	void* ptr = malloc(size);
	char key[64];
	snprintf(key, sizeof(key), "%p", ptr);

#define LEN 256

	char* val = (char*) malloc(LEN);
	snprintf(val, LEN, "%s(%d),%s", file, line, func);

	if (__lock) {  // xxx
		acl_pthread_mutex_lock(__lock);
	}

	if (__addrs == NULL || __mapper == NULL) {
		if (__lock) {
			acl_pthread_mutex_unlock(__lock);
		}
		return ptr;
	}

	acl_htable_enter(__addrs, key, val);

	int* counter = (int*) acl_htable_find(__mapper, val);
	if (counter) {
		(*counter)++;
	} else {
		counter = (int*) malloc(sizeof(int));
		*counter = 1;
		acl_htable_enter(__mapper, val, counter);
	}

	if (__lock) {
		acl_pthread_mutex_unlock(__lock);
	}
	return ptr;
}

static void free_mem(void* ptr)
{
	if (ptr == NULL) {
		return;
	}

	char key[64];
	snprintf(key, sizeof(key), "%p", ptr);
	free(ptr);

	if (__lock) {
		acl_pthread_mutex_lock(__lock);
	}

	if (__addrs == NULL || __mapper == NULL) {
		if (__lock) {
			acl_pthread_mutex_unlock(__lock);
		}
		return;
	}

	char* val = (char*) acl_htable_find(__addrs, key);
	if (val != NULL) {
		acl_htable_delete(__addrs, key, NULL);

		int* counter = (int*) acl_htable_find(__mapper, val);
		if (counter == NULL) {
			printf("not found val=%s, key=%s\r\n", val, key);
			abort();
		}

		--(*counter);
		if (*counter == 0) {
			acl_htable_delete(__mapper, val, NULL);
			free(counter);
		}
		free(val);
	}

	if (__lock) {
		acl_pthread_mutex_unlock(__lock);
	}
}

void operator delete(void* ptr) throw()
{
	free_mem(ptr);
}

void operator delete(void* ptr, size_t) throw()
{
	free_mem(ptr);
}

namespace acl {

void debug_mem_show(void)
{
	if (__addrs == NULL) {
		return;
	}

	printf("\r\n");
	ACL_ITER iter;

	if (__lock) {
		acl_pthread_mutex_lock(__lock);
	}

	acl_foreach(iter, __mapper) {
		const char* key = (const char*) iter.key;
		const int* counter = (const int*) iter.data;
		printf("%s --> %d\r\n", key, *counter);
	}

	if (__lock) {
		acl_pthread_mutex_lock(__lock);
	}

	printf("\r\n");
}

void debug_mem_start(void)
{
	acl_pthread_mutex_t* lock = (acl_pthread_mutex_t*)
		calloc(1, sizeof(acl_pthread_mutex_t));
	acl_pthread_mutex_init(lock, NULL);

	acl_pthread_mutex_lock(lock);
	__lock = lock;

	if (__addrs == NULL) {
		__addrs = acl_htable_create(100000, 0);
	}
	if (__mapper == NULL) {
		__mapper = acl_htable_create(100000, 0);
	}

	acl_pthread_mutex_unlock(__lock);
}

} // namespace acl

#endif  // DEBUG_NEW

namespace acl
{

static ACL_MEM_SLICE* __mem_slice = NULL;

void  acl_slice_init(void)
{
	__mem_slice = acl_mem_slice_init(8, 10240, 100000,
		ACL_SLICE_FLAG_GC2 | ACL_SLICE_FLAG_RTGC_OFF
		| ACL_SLICE_FLAG_LP64_ALIGN);
}

void* acl_new(size_t size, const char* filename,
	const char* funcname acl_unused, int lineno)
{
	void* ptr = acl_malloc_glue(filename, lineno, size);
	return (ptr);
}

void  acl_delete(void *ptr, const char* filename,
	const char* funcname acl_unused, int lineno)
{
	if (ptr == NULL) {
		return;
	}
	acl_free_glue(filename, lineno, ptr);
}

}  // namespace acl
