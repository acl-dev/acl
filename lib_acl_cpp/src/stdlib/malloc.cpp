#include "acl_stdafx.hpp"
#include <new>
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/malloc.hpp"
#endif

#ifdef HOOK_NEW

#ifdef ACL_WINDOWS

# ifdef NDEBUG
void* operator new(size_t n)
{
	return acl::acl_new(n, __FILE__, __FUNCTION__, __LINE__);
}

void  operator delete(void *p)
{
	acl::acl_delete(p, __FILE__, __FUNCTION__, __LINE__);
}
# endif

#else  // ACL_WINDOWS

void* operator new(size_t n) throw (std::bad_alloc)
{
	return acl::acl_new(n, __FILE__, __FUNCTION__, __LINE__);
}
void  operator delete(void *p) throw()
{
	acl::acl_delete(p, __FILE__, __FUNCTION__, __LINE__);
}

#endif  // !ACL_WINDOWS

#endif  // HOOK_NEW

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
	if (ptr == NULL)
		return;
	acl_free_glue(filename, lineno, ptr);
}

}  // namespace acl
