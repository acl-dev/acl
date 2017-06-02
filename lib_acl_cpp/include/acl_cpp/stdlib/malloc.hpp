#pragma once
#include "../acl_cpp_define.hpp"

#ifdef HOOK_NEW

#if defined(_WIN32) || defined(_WIN64)
# ifdef NDEBUG
void* operator new(size_t n);
void  operator delete(void *p);
# endif
#else
#include <new>
void* operator new(size_t n)  throw (std::bad_alloc);
void  operator delete(void *p) throw();
#endif

#endif

namespace acl {

ACL_CPP_API void  acl_slice_init(void);

/**
 * 内存分配函数
 * @param size {size_t} 需要分配的尺寸大小
 * @param filename {const char*} 源程序名字
 * @param funcname {const char*} 函数名
 * @param lineno {int} 源程序行号
 * @return {void*} 分配的内存地址，如果分配内存失败，则直接abort
 */
ACL_CPP_API void* acl_new(size_t size, const char* filename,
	const char* funcname, int lineno);


/**
 * 释放内存函数
 * @param ptr {void*} 内存地址
 * @param filename {const char*} 源程序名字
 * @param funcname {const char*} 函数名
 * @param lineno {int} 源程序行号
 */
ACL_CPP_API void  acl_delete(void *ptr, const char* filename,
	const char* funcname, int lineno);

}
