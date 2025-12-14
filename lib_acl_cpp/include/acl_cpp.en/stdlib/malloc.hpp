#pragma once
#include "../acl_cpp_define.hpp"

#ifdef ACL_HOOK_NEW

# if defined(_WIN32) || defined(_WIN64)
#  ifdef NDEBUG
void* operator new(size_t n);
void  operator delete(void *p);
#  endif
# else
#include <new>
void* operator new(size_t n)  throw (std::bad_alloc);
void  operator delete(void *p) throw();
# endif

#elif defined(ACL_CPP_DEBUG_MEM)

/**
 * When checking for memory leaks in memory allocation in acl library, by
 * overloading new/delete
 * to record the creation and destruction process of objects. If an object is
 * created but not released, it will be internally marked as
 * a memory leak. Users can periodically output these address information to see
 * which objects created in which files were not
 * released. To use this feature, need to enable the macro "//#define
 * ACL_CPP_DEBUG_MEM" in lib_acl_cpp/include/acl_cpp/acl_cpp_define.hpp,
 * and applications need to include the header file in their source code:
 * #include "acl_cpp/stdlib/malloc.hpp
 * Finally, applications should use NEW instead of new when creating objects.
 */
void* operator new(size_t, const char*, const char*, int) throw();
void operator delete(void*) throw();
void operator delete(void*, size_t) throw();

#define NEW new(__FILE__, __FUNCTION__, __LINE__)

namespace acl {

/**
 * Start memory monitoring process
 */
ACL_CPP_API void mem_checker_start(const char* logfile = NULL);

/**
 * Display current memory status to screen. Users can add timers to periodically
 * call this function
 */
ACL_CPP_API void mem_checker_show(void);
}

#endif

namespace acl {

ACL_CPP_API void  acl_slice_init(void);

/**
 * Memory allocation function
 * @param size {size_t} Size to allocate
 * @param filename {const char*} Source program name
 * @param funcname {const char*} Function name
 * @param lineno {int} Source program line number
 * @return {void*} Allocated memory address. If memory allocation fails,
 * directly abort
 */
ACL_CPP_API void* acl_new(size_t size, const char* filename,
	const char* funcname, int lineno);


/**
 * Memory deallocation function
 * @param ptr {void*} Memory address
 * @param filename {const char*} Source program name
 * @param funcname {const char*} Function name
 * @param lineno {int} Source program line number
 */
ACL_CPP_API void  acl_delete(void *ptr, const char* filename,
	const char* funcname, int lineno);

}

