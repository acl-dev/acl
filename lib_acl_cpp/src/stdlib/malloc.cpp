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

static acl_pthread_once_t __checker_once = ACL_PTHREAD_ONCE_INIT;

static void init_checker_once(void)
{
	__lock = (acl_pthread_mutex_t*) calloc(1, sizeof(acl_pthread_mutex_t));
	if (acl_pthread_mutex_init(__lock, NULL) != 0) {
		printf("%s: pthread_mutex_init error", __FUNCTION__);
		abort();
	}
}

static void mem_checker_once(void)
{
	if (acl_pthread_once(&__checker_once, init_checker_once) != 0) {
		printf("%s: pthread_once error\r\n", __FUNCTION__);
		abort();
	}

	if (__lock == NULL) {
		printf("%s: __lock NULL\r\n", __FUNCTION__);
		abort();
	}
}

static void mem_checker_lock(void)
{
	if (acl_pthread_mutex_lock(__lock) != 0) {
		printf("%s: pthread_mutex_lock error", __FUNCTION__);
		abort();
	}
}

static void mem_checker_unlock(void)
{
	if (acl_pthread_mutex_unlock(__lock) != 0) {
		printf("%s: pthread_mutex_unlock error", __FUNCTION__);
		abort();
	}
}

void* operator new(size_t size, const char* file, const char* func,
	int line) throw()
{
	void* ptr = malloc(size);

	mem_checker_once();
	mem_checker_lock();

	if (__addrs == NULL || __mapper == NULL) {
		mem_checker_unlock();
		return ptr;
	}

#define LEN 256

	char key[64];
	snprintf(key, sizeof(key), "%p", ptr);

	char* val = (char*) malloc(LEN);
	snprintf(val, LEN, "%s(%d),%s", file, line, func);

	acl_htable_enter(__addrs, key, val);

	int* counter = (int*) acl_htable_find(__mapper, val);
	if (counter) {
		(*counter)++;
	} else {
		counter = (int*) malloc(sizeof(int));
		*counter = 1;
		acl_htable_enter(__mapper, val, counter);
	}

	mem_checker_unlock();
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

	mem_checker_once();
	mem_checker_lock();

	if (__addrs == NULL || __mapper == NULL) {
		mem_checker_unlock();
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

	mem_checker_unlock();
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

void mem_checker_show(void)
{
	ACL_ITER iter;

	mem_checker_once();
	mem_checker_lock();

	if (__addrs == NULL) {
		mem_checker_unlock();
		return;
	}

	printf("\r\n");
	acl_foreach(iter, __mapper) {
		const char* key    = (const char*) iter.key;
		const int* counter = (const int*) iter.data;
		printf("%s --> %d\r\n", key, *counter);
	}

	mem_checker_unlock();
	printf("\r\n");
}

void mem_checker_start(void)
{
	mem_checker_once();
	mem_checker_lock();

	if (__addrs == NULL) {
		__addrs  = acl_htable_create(100000, 0);
	}
	if (__mapper == NULL) {
		__mapper = acl_htable_create(100000, 0);
	}

	mem_checker_unlock();
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
