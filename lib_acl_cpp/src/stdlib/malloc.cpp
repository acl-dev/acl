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

class mem_debug_lock {
public:
	mem_debug_lock(void) {
		mutex_ = (acl_pthread_mutex_t*)
			calloc(1, sizeof(acl_pthread_mutex_t));

#ifdef	ACL_UNIX
		pthread_mutexattr_init(&mutex_attr_);
		pthread_mutexattr_settype(&mutex_attr_, PTHREAD_MUTEX_RECURSIVE);
		(void) acl_pthread_mutex_init(mutex_, &mutex_attr_);
#else
		(void) acl_pthread_mutex_init(mutex_, NULL);
#endif
	}

	~mem_debug_lock(void) {
		// xxx: don't free mutex_ here !!!
	}

	void lock(void) {
		(void) acl_pthread_mutex_lock(mutex_);
	}

	void unlock(void) {
		(void) acl_pthread_mutex_unlock(mutex_);
	}

private:
	acl_pthread_mutex_t* mutex_;
#if !defined(_WIN32) && !defined(_WIN64)
	pthread_mutexattr_t  mutex_attr_;
#endif
};

static ACL_HTABLE* __addrs = NULL;
static ACL_HTABLE* __mapper = NULL;
static mem_debug_lock __lock;

void* operator new(size_t size, const char* file, const char* func,
	int line) throw()
{

	void* ptr = malloc(size);
	char key[64];
	snprintf(key, sizeof(key), "%p", ptr);

#define LEN 256

	char* val = (char*) malloc(LEN);
	snprintf(val, LEN, "%s(%d),%s", file, line, func);

	__lock.lock();

	if (__addrs == NULL || __mapper == NULL) {
		__lock.unlock();
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

	__lock.unlock();
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

	__lock.lock();

	if (__addrs == NULL || __mapper == NULL) {
		__lock.unlock();
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

	__lock.unlock();
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

	__lock.lock();

	acl_foreach(iter, __mapper) {
		const char* key    = (const char*) iter.key;
		const int* counter = (const int*) iter.data;
		printf("%s --> %d\r\n", key, *counter);
	}

	__lock.unlock();

	printf("\r\n");
}

void debug_mem_start(void)
{
	__lock.lock();

	__addrs  = acl_htable_create(100000, 0);
	__mapper = acl_htable_create(100000, 0);

	__lock.unlock();
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
